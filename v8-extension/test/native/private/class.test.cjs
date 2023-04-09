const assert = require('node:assert');
const { resolve: resolvePath } = require('node:path');
const native = require(resolvePath(process.env.JS_COMPILED_MODULE_PATH, 'native.node'));

assert(typeof native.Private === 'function');
assert.throws(() => {
    native.Private();
}, TypeError, 'Illegal constructor');
assert.throws(() => {
    new native.Private(5);
}, TypeError);
assert.throws(() => {
    new native.Private(Symbol('test'));
}, TypeError);
assert.doesNotThrow(() => {
    new native.Private();
    new native.Private('test');
});