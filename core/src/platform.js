import { nativeFunction, globalOf, getSecurityToken, setSecurityToken, setFunctionName } from '@dragiyski/v8-extensions';
import { createContext, runInContext, compileFunction, Script } from 'node:vm';
import Namespace from './namespace.js';
import { copyPrimordials } from './primordials.js';

export default class Platform extends Namespace {
    static #globalMap = new WeakMap();
    static #stack = [];
    static realm = Object.create(null);
    #context;
    #execute;
    #executionState;
    #securityStack = [];
    #unlockToken;
    #lockToken;
    #captureStackTrace;
    #nativeError;
    #thrown = new WeakSet();
    #expose;

    constructor(options) {
        super();
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
        let globalName = options.name;
        if (typeof exposeType !== 'string' || globalName.length <= 0) {
            globalName = 'Window';
        }
        let exposeTypes = options.expose;
        if (exposeTypes === Object(exposeTypes) && typeof exposeTypes[Symbol.iterator] === 'function') {
            exposeTypes = [...exposeTypes];
        } else {
            exposeTypes = [globalName];
        }
        this.#expose = Object.create(null);
        for (const exposeName of exposeTypes) {
            this.#expose[exposeName] = true;
        }
        // Ensure the global of the context is named.
        const NamedGlobal = nativeFunction(() => {}, { name: globalName });
        Object.setPrototypeOf(NamedGlobal.prototype, null);
        // The global will still have constructor, but it will be from its context's Function prototype.
        delete NamedGlobal.prototype.constructor;
        const contextGlobal = new NamedGlobal();
        this.#context = createContext(contextGlobal, contextOptions);
        const realmGlobal = runInContext(`globalThis`, this.#context);
        // Since the constructor will be different from this context Function.prototype, it won't have a name, so rename it.
        setFunctionName(realmGlobal.constructor, globalName);
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
        this.#nativeError = Object.getOwnPropertyNames(realmGlobal)
            .filter(name => (
                typeof realmGlobal[name] === 'function' &&
                this.call(this.primordials['Function.prototype.[Symbol.hasInstance]'], this.primordials.Error, realmGlobal[name].prototype)
            ));
        this.#nativeError.push('Error');
        this.#captureStackTrace = this.compileFunction(`platform.enterLock();
try {
    platform.call(platform.primordials['Error.captureStackTrace'], platform.primordials.Error, object, constructor);
    return object;
} finally {
    platform.leaveLock();
}`, ['platform', 'object', 'constructor']);
    }

    is(name) {
        return name in this.#expose;
    }

    getRealm() {
        return this.implementationOf(this.realm);
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
            top.#execute = top.#enterExecuteState;
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
                    if (Platform.#stack.length > 0) {
                        Platform.#stack[0].#execute = Platform.#stack[0].#directExecuteState;
                    }
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

    // Note: VM's contextified sandbox is a special native object, that:
    // 1. Everything assigned to the global will actually be assigned to the contextified sandbox, but will appear in global as own property.
    // (i.e. Global is a GlobalProxy with target contextified sandbox)
    // 2. When a context function in this context is invoked from the other context (including natively),
    // and the "this" is the global (including invocation without "this"), the "this" becomes contextified sandbox.
    // We have to correct for that.
    #contextSelf(self) {
        if (self === this.#context) {
            return this.global;
        }
        return self;
    }

    createNativeFunction(callee, options) {
        if (typeof options === 'function' && (callee == null || typeof callee === 'object')) {
            const opt = callee;
            callee = options;
            options = opt;
        }
        if (typeof callee !== 'function') {
            throw new TypeError('callee: not a function');
        }
        return nativeFunction(
            (self, args, newTarget) => this.#execute(callee, this.#contextSelf(self), args, newTarget),
            Object.assign(Object.create(null), { ...options, context: this.global })
        );
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

    // Note: Only here exceptions need to be remapped to guarantee user code has no access to the current platform.
    // Note: Calling user code does not require remapping. If unhandled exception is native, it can safely display the entire stack.
    // Warning: If exception is not native, and does not have implementation, it must be read with care. User code can install getters,
    // or modify it in a way that executes user code without locking the environment.
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
        return callee(this, ...args);
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
        let prototype = Object.getPrototypeOf(e);
        const errors = this.#nativeError.map(name => globalThis[name].prototype);
        let name = 'Error';
        while (prototype != null) {
            const index = errors.indexOf(prototype);
            if (index >= 0) {
                name = this.#nativeError[index];
                break;
            }
            prototype = Object.getPrototypeOf(prototype);
        }
        if (name === 'AggregateError') {
            return this.remapAggregateError(e);
        }
        return this.remapGenericError(e, name);
    }

    // AggregateError may contain multiple errors.
    // It is expected to only be thrown automatically from Promise.any() when all promises are rejected.
    // Unlike other errors, this requires as first parameters array/iterable of errors, the message goes into the second argument.
    remapAggregateError(errorImpl) {
        const args = [];
        const errors = errorImpl.errors;
        if (errors === Object(errors) && typeof errors[Symbol.iterator] === 'function') {
            args[0] = [...errors].map(error => this.remapException(error));
        } else {
            args[0] = [];
        }
        const message = errorImpl.message;
        if (typeof message === 'string' && message.length > 0) {
            args[1] = message;
        }
        const cause = errorImpl.cause;
        if (cause != null) {
            args[2] = {
                cause: this.remapException(cause)
            };
        }
        const errorInterface = new this.primordials.AggregateError(...args);
        this.setImplementation(errorInterface, errorImpl);
        this.captureStackTrace(errorInterface);
        return errorInterface;
    }

    remapGenericError(errorImpl, name) {
        const args = [];
        if (errorImpl instanceof Error) {
            const message = errorImpl.message;
            if (typeof message === 'string' && message.length > 0) {
                args[0] = message;
            }
            const cause = errorImpl.cause;
            if (cause != null) {
                args[1] = {
                    cause: this.remapException(cause)
                };
            }
        }
        const errorInterface = new this.primordials[name](...args);
        this.setImplementation(errorInterface, errorImpl);
        this.captureStackTrace(errorInterface);
        return errorInterface;
    }

    throw(error) {
        this.#thrown.add(error);
        throw error;
    }

    isThrown(error) {
        return this.#thrown.has(error);
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
            throw new Platform.SecurityStateError('Attempting to leave non-existent state');
        }
        const top = this.#securityStack[0];
        if (!top.lock) {
            throw new Platform.SecurityStateError('Attempting to leave locked state of unlocked platform');
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

    callUserFunction(target, thisArg, args) {
        Platform.enter(this);
        this.enterLock();
        try {
            return Reflect.apply(target, thisArg, args);
        } finally {
            this.leaveLock();
            Platform.leave(this);
        }
    }

    runUserScript(script, options) {
        if (!(script instanceof Script)) {
            throw new TypeError('Expected arguments[0] to be instance of vm.Script (module node:vm)');
        }
        options = { ...options };
        Platform.enter(this);
        this.enterLock();
        try {
            return script.runInContext(this.#context, options);
        } finally {
            this.leaveLock();
            Platform.leave(this);
        }
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
