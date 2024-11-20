const assert = require('node:assert');
const { resolve: resolvePath } = require('node:path');
const native = require(resolvePath(process.env.JS_COMPILED_MODULE_PATH, 'native.node'));

const o = Reflect.construct(function () {}, [], native.Private);
const q = {};
console.error(o.get);
assert.throws(() => {
    o.get(q);
}, TypeError);
assert.throws(() => {
    o.has(q);
}, TypeError);
assert.throws(() => {
    o.set(q, 'test');
}, TypeError);
assert.throws(() => {
    o.delete(q);
}, TypeError);
