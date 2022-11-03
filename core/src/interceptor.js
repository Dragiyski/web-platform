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
            throw new TypeError('Illegal invocation');
        }
        return callee(platform, self, ...more);
    };
};

export function requireThisOwnImplementationInterceptor(callee) {
    return function interceptor(platform, self, ...more) {
        self = platform.ownImplementationOf(self);
        if (self == null) {
            throw new TypeError('Illegal invocation');
        }
        return callee(platform, self, ...more);
    };
};

export function validateThisImplementationInterceptor(callee, Class) {
    return function interceptor(platform, self, ...more) {
        self = platform.implementationOf(self);
        if (!(self instanceof Class)) {
            throw new TypeError('Illegal invocation');
        }
        return callee(platform, self, ...more);
    };
}

export function validateThisOwnImplementationInterceptor(callee, Class) {
    return function interceptor(platform, self, ...more) {
        self = platform.ownImplementationOf(self);
        if (!(self instanceof Class)) {
            throw new TypeError('Illegal invocation');
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
    return function interceptor() {
        return this[name];
    };
}

export function setter(name) {
    return function interceptor(value) {
        this[name] = value;
    };
}

export function caller(name) {
    return function interceptor(...parameters) {
        return this[name](...parameters);
    };
}

export function constructorErrorPrefixInterceptor(className, callee) {
    return function interceptor(platform, ...rest) {
        try {
            return callee(platform, ...rest);
        } catch (e) {
            if (e !== Object(e)) {
                throw e;
            }
            if (platform.globalOf(e) === platform.global) {
                throw e;
            }
            throw e;
        }
    };
}

export function defaultConstructor() {
    return function (platform, self, parameters, target) {
        const Implementation = platform.implementationOf(target);
        if (typeof Implementation !== 'function') {
            throw new TypeError('Illegal constructor');
        }
        const objectImpl = new Implementation(...parameters);
        const objectInterface = Object.create(platform.interfaceOf(Implementation.prototype));
        platform.setImplementation(objectInterface, objectImpl);
        return objectImpl;
    };
}
