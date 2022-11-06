import messages from './messages.js';

export function requireNewTargetInterceptor(callee) {
    return function interceptor(platform, thisArg, args, newTarget) {
        if (newTarget == null) {
            platform.throw(new TypeError(messages.newOperatorRequired()));
        }
        return callee(platform, thisArg, args, newTarget);
    };
}
