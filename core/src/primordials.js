export function copyPrimordials(primordials, global) {
    copyAll(primordials, '', global);
}

function copyAll(primordials, prefix, source, ...stack) {
    if (stack.indexOf(source) >= 0) {
        return;
    }
    for (const name of Object.getOwnPropertyNames(source)) {
        const descriptor = Object.getOwnPropertyDescriptor(source, name);
        copyProperty(primordials, `${prefix}${name}`, descriptor);
        if (descriptor.value === Object(descriptor.value)) {
            copyAll(primordials, `${prefix}${name}.`, descriptor.value, ...stack, source);
        }
    }
}

function copyProperty(primordials, primordialName, descriptor) {
    if (Object.prototype.hasOwnProperty.call(descriptor, 'value')) {
        Object.defineProperty(primordials, primordialName, {
            configurable: true,
            enumerable: true,
            value: descriptor.value
        });
    } else if (Object.prototype.hasOwnProperty.call(descriptor, 'get')) {
        const getter = descriptor.get;
        Object.defineProperty(primordials, `${primordialName}[[get]]`, {
            configurable: true,
            enumerable: true,
            value: getter
        });
        if (Object.prototype.hasOwnProperty.call(descriptor, 'set')) {
            const setter = descriptor.set;
            Object.defineProperty(primordials, `${primordialName}[[set]]`, {
                configurable: true,
                enumerable: true,
                value: setter
            });
        }
    }
}

export function definePrimordial(primordials, name, value) {
    copyProperty(primordials, name, { value });
    copyAll(primordials, `${name}.`, value);
}
