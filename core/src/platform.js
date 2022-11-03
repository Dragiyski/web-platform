import { nativeFunction, globalOf, getSecurityToken, setSecurityToken } from '@dragiyski/v8-extensions';
import { createContext, runInContext, compileFunction } from 'node:vm';
import { copyPrimordials } from './primordials.js';
import { thrown } from './symbols.js';

export default class Platform {
    static #globalMap = new WeakMap();
    static #stack = [];
    static realm = Object.create(null);
    #interface = Symbol('interface');
    #implementation = new WeakMap();
    #context;
    #execute;
    #executionState;
    #securityStack = [];
    #unlockToken;
    #lockToken;
    #captureStackTrace;

    constructor(options) {
        options ??= {};
        this.#executionState = this.#executeDirect;
        this.#execute = this.#enterExecuteState;
        const contextOptions = {};
        {
            const value = options.contextName;
            if (value != null) {
                contextOptions.name = '' + value;
            }
        }
        {
            let value = options.contextOrigin;
            if (value != null) {
                if (value instanceof URL) {
                    value = value.origin;
                }
                contextOptions.origin = '' + value;
            }
        }
        this.#context = createContext(Object.create(null), contextOptions);
        const realmGlobal = runInContext(`globalThis`, this.#context);
        const realmObject = Object.create(null);
        this.#unlockToken = getSecurityToken(realmGlobal);
        this.#lockToken = Object.create(null);
        Object.defineProperties(this, {
            global: {
                value: realmGlobal
            },
            realm: {
                value: realmObject
            },
            primordials: {
                value: Object.create(null)
            }
        });
        Platform.#globalMap.set(realmGlobal, this);
        this.setImplementation(realmGlobal, Object.create(null));
        this.setImplementation(realmObject, Object.create(null));
        copyPrimordials(this.primordials, this.global);
        this.#captureStackTrace = this.compileFunction(`platform.enterLock();
try {
    platform.call(platform.primordials['Error.captureStackTrace'], platform.primordials.Error, object, constructor);
    return object;
} finally {
    platform.leaveLock();
}`, ['platform', 'object', 'constructor']);
    }

    captureStackTrace(object, constructor) {
        if (constructor == null) {
            constructor = this.#captureStackTrace;
        }
        if (object == null) {
            object = Object.create(null);
        }
        const captureStackTrace = this.#captureStackTrace;
        captureStackTrace(this, object, constructor);
        return object.stack;
    }

    static enter(platform) {
        if (!(platform instanceof Platform)) {
            throw new TypeError('Invalid `arguments[0]`: expected `Platform` instance');
        }
        if (Platform.#stack.length > 0) {
            const top = Platform.#stack[0];
            if (top.platform === platform) {
                ++top.ref;
                return this;
            }
        }
        this.#stack.unshift({
            platform,
            ref: 1
        });
        platform.#execute = platform.#directExecuteState;
        return this;
    }

    static leave(platform) {
        if (Platform.#stack.length > 0) {
            const top = Platform.#stack[0];
            if (top.platform === platform) {
                if (--top.ref <= 0) {
                    platform.#execute = platform.#enterExecuteState;
                    Platform.#stack.shift();
                }
                return this;
            }
        }
        throw new ReferenceError(`Platform stack violation`);
    }

    /**
     * @returns {Platform}
     */
    static current() {
        if (Platform.#stack.length > 0) {
            const top = Platform.#stack[0];
            return top.platform;
        }
        throw new Platform.RuntimeError('Current execution context is not in a web platform');
    }

    static fromObject(object) {
        if (object !== Object(object)) {
            return null;
        }
        const global = globalOf(object);
        if (global != null && Platform.#globalMap.has(global)) {
            return Platform.#globalMap.get(global);
        }
        return null;
    }

    createNativeFunction(callee, options) {
        return nativeFunction((...args) => this.#execute(callee, ...args), Object.assign(Object.create(null), options));
    }

    createObject(prototype) {
        const object = Object.create(prototype);
        const relatedInterface = this.interfaceOf(prototype);
        if (relatedInterface != null) {
            const interfaceObject = Object.create(relatedInterface);
            this.setImplementation(interfaceObject, object);
        }
        return object;
    }

    createInterface(Implementation, ...rest) {
        if (typeof Implementation !== 'function') {
            throw new TypeError('Expected arguments[1] to be a function');
        }
        const Interface = this.createNativeFunction(...rest);
        this.setImplementation(Interface, Implementation);
        this.setImplementation(Interface.prototype, Implementation.prototype);
        return Interface;
    }

    compileFunction(code, params = [], options = {}) {
        return compileFunction(code, params, {
            ...options,
            parsingContext: this.#context
        });
    }

    // Methods below provide secure way for locked code with access to environment to call functions,
    // without relying to potentially overridden Function.prototype.* in the user context.

    call(func, ...args) {
        return Function.prototype.call.call(func, ...args);
    }

    apply(func, ...args) {
        return Function.prototype.apply.call(func, ...args);
    }

    bind(func, ...args) {
        return Function.prototype.bind.call(func, ...args);
    }

    // Call "native" function from user code, unlocking the platform first.
    // While the platform is in locked state, its security token does not match the NodeJS main context security token.
    // This prevent access to certain object properties, filter stack trace frames to those in the user context, etc.
    // Unless explicitly exposed, the user code cannot access any part of the Node environment (without generating "no access" TypeError),
    // nor it can use stack trace to detect it runs in NodeJS, not in a browser.
    #executeLocked(callee, ...args) {
        this.enterUnlock();
        try {
            return callee(this, ...args);
        } catch (e) {
            throw this.remapException(e);
        } finally {
            this.leaveUnlock();
        }
    }

    // Call "native" (implementation) function directly.
    // This must only be used for unlocked state, where one "native" function can call directly another "native" function.
    // The stack trace include everything.
    #executeDirect(callee, ...args) {
        try {
            return callee(this, ...args);
        } catch (e) {
            throw this.remapException(e);
        }
    }

    remapException(e) {
        if (Platform.fromObject(e) === this) {
            // Case 1: Not an object;
            // Case 2: An object that is created within the platform context;
            // No remapping is done.
            return e;
        }
        if (this.hasInterface(e)) {
            // Case 3: This is an implementation of an error;
            return this.interfaceOf(e);
        }
        const prototypeChain = [];
        let o = Object.getPrototypeOf(e);
        while (o != null) {
            prototypeChain.push(o);
            o = Object.getPrototypeOf(o);
        }
        if (prototypeChain.indexOf(AggregateError.prototype) >= 0) {
            return this.remapAggregateError(e);
        }
        return this.remapGenericError(e);
    }

    remapAggregateError(errorImpl) {
        let errorInterface = null;
        if (!(errorImpl instanceof Error)) {
            errorInterface = new this.primordials.Error();
        }
        const options = {};
        if (errorImpl.cause != null) {
            options.cause = this.remapException(errorImpl.cause);
        }
        errorInterface = new this.primordials.AggregateError(errorImpl.errors, errorImpl.message, options);
    }

    #directExecuteState(...args) {
        return this.#executionState(...args);
    }

    #enterExecuteState(...args) {
        Platform.enter(this);
        try {
            return this.#executionState(...args);
        } finally {
            Platform.leave(this);
        }
    }

    enterLock() {
        if (this.#securityStack.length > 0) {
            const top = this.#securityStack[0];
            if (top.lock) {
                top.ref++;
                return this;
            }
        }
        this.#securityStack.unshift({
            lock: true,
            ref: 1
        });
        setSecurityToken(this.global, this.#lockToken);
        this.#executionState = this.#executeLocked;
    }

    leaveLock() {
        if (this.#securityStack.length <= 0) {
            throw Platform.SecurityStateError('Attempting to leave non-existent state');
        }
        const top = this.#securityStack[0];
        if (!top.lock) {
            throw Platform.SecurityStateError('Attempting to leave locked state of unlocked platform');
        }
        if (--top.ref <= 0) {
            this.#securityStack.shift();
            if (this.#securityStack.length > 0 && this.#securityStack[0].lock) {
                return this;
            }
            setSecurityToken(this.global, this.#unlockToken);
            this.#executionState = this.#executeDirect;
        }
        return this;
    }

    enterUnlock() {
        if (this.#securityStack.length > 0) {
            const top = this.#securityStack[0];
            if (!top.lock) {
                top.ref++;
                return this;
            }
        }
        this.#securityStack.unshift({
            lock: false,
            ref: 1
        });
        setSecurityToken(this.global, this.#unlockToken);
        this.#executionState = this.#executeDirect;
    }

    leaveUnlock() {
        if (this.#securityStack.length <= 0) {
            throw Platform.SecurityStateError('Attempting to leave non-existent state');
        }
        const top = this.#securityStack[0];
        if (top.lock) {
            throw Platform.SecurityStateError('Attempting to leave unlocked state of locked platform');
        }
        if (--top.ref <= 0) {
            this.#securityStack.shift();
            if (this.#securityStack.length > 0 && this.#securityStack[0].lock) {
                setSecurityToken(this.global, this.#lockToken);
                this.#executionState = this.#executeLocked;
            }
            // When the security stack is empty, the platform is left unlocked.
        }
        return this;
    }

    ownInterfaceOf(object) {
        if (object !== Object(object)) {
            return null;
        }
        if (Object.prototype.hasOwnProperty.call(object, this.#interface)) {
            return object[this.#interface];
        }
        return null;
    }

    ownImplementationOf(object) {
        if (object !== Object(object)) {
            return null;
        }
        if (this.#implementation.has(object)) {
            return this.#implementation.get(object);
        }
        return null;
    }

    interfaceOf(object) {
        if (object !== Object(object)) {
            return null;
        }
        if (this.#interface in object) {
            return object[this.#interface];
        }
        return null;
    }

    implementationOf(object) {
        if (object !== Object(object)) {
            return null;
        }
        while (object != null) {
            if (this.#implementation.has(object)) {
                return this.#implementation.get(object);
            }
            object = Object.getPrototypeOf(object);
        }
        return null;
    }

    hasOwnImplementation(object) {
        if (object !== Object(object)) {
            return false;
        }
        return this.#implementation.has(object);
    }

    hasOwnInterface(object) {
        if (object !== Object(object)) {
            return false;
        }
        return Object.prototype.hasOwnProperty.call(object, this.#interface);
    }

    hasImplementation(object) {
        if (object !== Object(object)) {
            return false;
        }
        while (object != null) {
            if (this.#implementation.has(object)) {
                return true;
            }
            object = Object.getPrototypeOf(object);
        }
        return false;
    }

    hasInterface(object) {
        if (object !== Object(object)) {
            return false;
        }
        return this.#interface in object;
    }

    removeImplementationOf(object) {
        if (object !== Object(object)) {
            return this;
        }
        if (this.#implementation.has(object)) {
            const implementationObject = this.#implementation.get(object);
            this.#implementation.delete(object);
            delete implementationObject[this.#interface];
        }
        return this;
    }

    removeInterfaceOf(object) {
        if (object !== Object(object)) {
            return this;
        }
        if (Object.prototype.hasOwnProperty.call(object, this.#interface)) {
            const interfaceObject = object[this.#interface];
            this.#implementation.delete(interfaceObject);
            delete object[this.#interface];
        }
        return this;
    }

    setImplementation(interfaceObject, implementationObject) {
        if (interfaceObject !== Object(interfaceObject) || implementationObject !== Object(implementationObject)) {
            throw new TypeError(`Implementation linking can only be realized between two objects`);
        }
        if (this.hasOwnImplementation(interfaceObject)) {
            const otherImplementation = this.ownImplementationOf(interfaceObject);
            if (otherImplementation === implementationObject) {
                return this;
            }
            throw new ReferenceError(`interfaceObject: already has an implementation in this platform`);
        }
        if (this.hasOwnInterface(implementationObject)) {
            const otherInterface = this.ownInterfaceOf(implementationObject);
            if (otherInterface === interfaceObject) {
                return this;
            }
            throw new ReferenceError(`implementationObject: already has an interface in this platform`);
        }
        this.#implementation.set(interfaceObject, implementationObject);
        implementationObject[this.#interface] = interfaceObject;
        return this;
    }

    /**
     * @todo For platforms supporting locking, this must lock before the function call and unlock after that.
     * @param {Function} target
     * @param {*} thisArg
     * @param {Array} args
     * @returns
     */
    callUserFunction(target, thisArg, args) {
        return Reflect.apply(target, thisArg, args);
    }
}

Platform.RuntimeError = class RuntimeError extends Error {
    name = 'Platform.RuntimeError';
};

Platform.SecurityStateError = class SecurityStateError extends Platform.RuntimeError {
    name = 'Platform.SecurityStateError';
};

{
    const assigner = new Proxy(Object.create(null), {
        defineProperty() {
            return false;
        },
        deleteProperty() {
            return false;
        },
        isExtensible() {
            return true;
        },
        preventExtensions() {
            return false;
        },
        get(target, name, receiver) {
            if (Object.prototype.hasOwnProperty.call(receiver, name)) {
                return receiver[name];
            }
            if (typeof name !== 'string') {
                return;
            }
            const path = name.split('.');
            let object = global;
            while (path.length > 1) {
                const property = path.shift();
                object = object[property];
                if (object !== Object(object)) {
                    return;
                }
            }
            if (path[0] in object) {
                const value = object[path[0]];
                Object.defineProperty(receiver, name, {
                    configurable: true,
                    writable: true,
                    value
                });
                return value;
            }
        },
        has(target, name) {
            if (typeof property !== 'string') {
                return false;
            }
            name = name.split('.');
            let object = global;
            while (name.length > 1) {
                const property = name.shift();
                object = object[property];
                if (object !== Object(object)) {
                    return false;
                }
            }
            return name[0] in object;
        }
    });
    Object.defineProperty(Platform.prototype, 'primordials', {
        value: Object.create(assigner)
    });
}

Platform.enter(new Platform());

/**
 * @typedef {Function} PlatformCallback
 * @param {Platform} platform
 */

/**
 * @typedef {Function} PlatformInitialize
 * @param {...string} namespace
 * @param {PlatformCallback} callback
 * @return {Function}
 */
