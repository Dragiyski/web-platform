import binding from './native.cjs';

export const getSecurityToken = binding.getSecurityToken;
export const setSecurityToken = binding.setSecurityToken;
export const useDefaultSecurityToken = binding.useDefaultSecurityToken;
export const globalOf = binding.globalOf;
export const getFunctionName = binding.getFunctionName;
export const Script = binding.Script;

export function setFunctionName(func, name = '') {
    name = '' + name;
    binding.setFunctionName(func, name);
}

export function createContext(name = '') {
    name = '' + name;
    return binding.createContext(name);
}

export function compileFunction(options) {
    options = { ...options };
    Object.setPrototypeOf(options, null);
    if (typeof options.code !== 'string') {
        throw new TypeError(`Required option "code" is missing or not a string.`);
    }
    const code = options.code;
    delete options.code;
    const args = [];
    const ext = [];
    let context;
    if (options.context != null) {
        if (options.context !== Object(options.context)) {
            throw new TypeError(`Option "context" not an object`);
        }
        context = options.context;
    } else {
        context = Object.create(null);
    }
    if (options.arguments != null) {
        if (options.arguments === Object(options.arguments) && typeof options.arguments[Symbol.iterator] === 'function') {
            args.push(...options.arguments);
        } else {
            throw new TypeError(`Option "arguments" not an array/iterable`);
        }
        if (!args.every(arg => typeof arg === 'string')) {
            throw new TypeError(`Option "arguments" must be array of strings`);
        }
    }
    if (options.closures != null) {
        if (options.closures === Object(options.closures) && typeof options.closures[Symbol.iterator] === 'function') {
            ext.push(...options.closures);
        } else {
            throw new TypeError(`Option "closures" not an array/iterable`);
        }
        if (!ext.every(closure => closure === Object(closure))) {
            throw new TypeError(`Option "closures" must be array of objects`);
        }
    }
    delete options.context;
    delete options.arguments;
    delete options.closures;
    return binding.compileFunction(context, args, ext, code, options);
}

export function nativeFunction(callback, options) {
    if (typeof callback !== 'function') {
        throw new TypeError(`Expected arguments[0] to be a function`);
    }
    options = { ...options };
    const creation_arguments = [callback, true, null, 0, null];
    const option_constructor = options.constructor;
    const option_name = options.name;
    const option_length = options.length;
    const option_context = options.context;
    if (option_constructor != null) {
        creation_arguments[1] = Boolean(option_constructor);
    }
    if (option_name != null) {
        const name = '' + option_name;
        if (name.length > 0) {
            creation_arguments[2] = option_name;
        }
    }
    if (option_length != null) {
        if (!Number.isSafeInteger(option_length) || option_length < 0 || option_length > 0x7FFFFFFF) {
            throw new TypeError(`Expected option "length" to be valid 32-bit integer: in range [0; ${0x7FFFFFFF}]`);
        }
        creation_arguments[3] = option_length;
    }
    if (option_context != null) {
        if (option_context !== Object(option_context)) {
            throw new TypeError(`Expected option "context" to be an object`);
        }
        creation_arguments[4] = option_context;
    }
    return binding.createNativeFunction(...creation_arguments);
}
