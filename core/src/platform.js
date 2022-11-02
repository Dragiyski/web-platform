import { nativeFunction, globalOf } from '@dragiyski/v8-extensions';
import { createContext, runInContext } from 'node:vm';
import { copyPrimordials } from './primordials.js';

export default class Platform {
    static #globalMap = new WeakMap();
    static #stack = [];
    static realm = Object.create(null);
    #interface = Symbol('interface');
    #implementation = new WeakMap();
    #context;
    #execute;
    #executionState;

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

    static create(...args) {
        const platform = new this(...args);
        platform.initialize();
        return platform;
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

    #executeLocked(callee, ...args) {
        this.enterUnlock();
        try {
            return callee(this, ...args);
        } finally {
            this.leaveUnlock();
        }
    }

    #executeDirect(callee, ...args) {
        return callee(this, ...args);
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
