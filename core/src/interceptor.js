import messages from './messages.js';

export function returnValueInterfaceInterceptor(callee) {
    return function interceptor(platform, ...rest) {
        const value = callee(platform, ...rest);
        return platform.interfaceOf(value) ?? value;
    };
}

export function returnValueOwnInterfaceInterceptor(callee) {
    return function interceptor(platform, ...rest) {
        const value = callee(platform, ...rest);
        return platform.ownInterfaceOf(value) ?? value;
    };
}

export function newTargetImplementationInterceptor(callee) {
    return function interceptor(platform, self, parameter, target, ...more) {
        target = platform.implementationOf(target);
        return callee(platform, self, parameter, target, ...more);
    };
}

export function newTargetOwnImplementationInterceptor(callee) {
    return function newTargetOwnImplementation(platform, self, parameter, target, ...more) {
        target = platform.ownImplementationOf(target);
        return callee(platform, self, parameter, target, ...more);
    };
}

export function requireThisImplementationInterceptor(callee) {
    return function interceptor(platform, self, ...more) {
        self = platform.implementationOf(self);
        if (self == null) {
            throw new TypeError(messages.illegalInvocation());
        }
        return callee(platform, self, ...more);
    };
};

export function requireThisOwnImplementationInterceptor(callee) {
    return function interceptor(platform, self, ...more) {
        self = platform.ownImplementationOf(self);
        if (self == null) {
            throw new TypeError(messages.illegalInvocation());
        }
        return callee(platform, self, ...more);
    };
};

export function validateThisImplementationInterceptor(getClass, callee) {
    return function interceptor(platform, self, ...more) {
        self = platform.implementationOf(self);
        if (!(self instanceof getClass())) {
            throw new TypeError(messages.illegalInvocation());
        }
        return callee(platform, self, ...more);
    };
}

export function validateThisOwnImplementationInterceptor(getClass, callee) {
    return function interceptor(platform, self, ...more) {
        self = platform.ownImplementationOf(self);
        if (!(self instanceof getClass())) {
            throw new TypeError(messages.illegalInvocation());
        }
        return callee(platform, self, ...more);
    };
}

export function functionInterceptor(callee) {
    return function interceptor(platform, self, args) {
        return Reflect.apply(callee, self, args);
    };
}

export function getter(name) {
    return function getter() {
        return this[name];
    };
}

export function setter(name) {
    return function setter(value) {
        this[name] = value;
    };
}

export function caller(name) {
    return function caller(...parameters) {
        return this[name](...parameters);
    };
}

/**
 * Requires interceptor that unwrap the `Target` argument into `Implementation`.
 * For example: @link{validateConstructorTargetInterceptor}
 * @param {function} callee
 * @returns
 */
export function thisConstructor(callee) {
    return function constructor(platform, self, args, Implementation) {
        const implementation = Reflect.apply(callee, Implementation, args);
        platform.setImplementation(self, implementation);
        return implementation;
    };
}

export function requireArgumentCountInterceptor(length, callee) {
    return function interceptor(platform, thisArg, args, ...rest) {
        if (args.length < length) {
            platform.throw(new TypeError(messages.insufficientArgumentCount(args.length, length)));
        }
        return callee(platform, thisArg, args, ...rest);
    };
}

export function constructorErrorMessageInterceptor(className, callee) {
    return function interceptor(platform, ...rest) {
        try {
            return callee(platform, ...rest);
        } catch (e) {
            if (platform.isThrown(e)) {
                const message = [messages.constructorFailedToExecute(className)];
                if (e.message) {
                    message.push(e.message);
                }
                e.message = message.join(': ');
            }
            throw e;
        }
    };
}

export function methodErrorMessageInterceptor(className, methodName, callee) {
    return function interceptor(platform, ...rest) {
        try {
            return callee(platform, ...rest);
        } catch (e) {
            if (platform.isThrown(e)) {
                const message = [messages.methodFailedToExecute(className, methodName)];
                if (e.message) {
                    message.push(e.message);
                }
                e.message = message.join(': ');
            }
            throw e;
        }
    };
}

export function returnArrayInterfaceInterceptor(callee) {
    return function interceptor(platform, ...rest) {
        const value = callee(platform, ...rest);
        const wrapped = new platform.primordials.Array();
        platform.call(platform.primordials['Array.prototype.push'], wrapped, [...value].map(item => platform.interfaceOf(item) ?? item));
        return wrapped;
    };
}

export function validateConstructorTargetInterceptor(getClass, callee) {
    return function interceptor(platform, self, args, Interface) {
        const Implementation = platform.implementationOf(Interface);
        if (typeof Implementation !== 'function') {
            throw new TypeError(messages.illegalConstructor());
        }
        const Class = getClass();
        if (Implementation !== Class && !(Implementation.prototype instanceof Class)) {
            throw new TypeError(messages.illegalConstructor());
        }
        return callee(platform, self, args, Implementation);
    };
}

export function requireArgumentTypeInterceptor(index, getClass, callee) {
    return function interceptor(platform, self, args, ...rest) {
        const value = platform.implementationOf(args[index]);
        const Class = getClass();
        if (!(value instanceof Class)) {
            platform.throw(new TypeError(messages.invalidArgumentType(index, Class.name)));
        }
        args[index] = value;
        return callee(platform, self, args, ...rest);
    };
}
