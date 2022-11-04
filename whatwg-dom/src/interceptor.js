export { interceptors as core } from '@dragiyski/web-platform-core';

export function validateNewTargetInterceptor(callee) {
    return function interceptor(platform, thisArg, args, newTarget) {
        if (newTarget == null) {
            platform.throw(new TypeError(`Please use the 'new' operator, this DOM object constructor cannot be called as a function.`));
        }
        return callee(platform, thisArg, args, newTarget);
    };
}
