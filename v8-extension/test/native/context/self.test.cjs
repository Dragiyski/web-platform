const assert = require('node:assert');
const { resolve: resolvePath } = require('node:path');
const native = require(resolvePath(process.env.JS_COMPILED_MODULE_PATH, 'native.node'));


assert(typeof native.Context === 'function');
assert('current' in native.Context)
assert(native.Context.current instanceof native.Context);

const currentContext = native.Context.current;
assert.strictEqual(currentContext, native.Context.current, 'Context.current return the same wrapper every time');
assert.strictEqual(currentContext.global, globalThis, 'Context.current.global === globalThis');

assert(typeof native.Context.for === 'function');
const objectFor = native.Context.for({});
assert.strictEqual(currentContext, objectFor, 'Context.for({}) === Context.current');
const globalFor = native.Context.for(globalThis);
assert.strictEqual(currentContext, globalFor, 'Context.for(globalThis) === Context.current');
