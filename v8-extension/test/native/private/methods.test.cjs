const assert = require('node:assert');
const { resolve: resolvePath } = require('node:path');
const native = require(resolvePath(process.env.JS_COMPILED_MODULE_PATH, 'native.node'));

const o = {};
const s = Symbol('test');
const p = new native.Private();
assert.strictEqual(p.has(o), false);
assert.strictEqual(p.get(o), undefined);
assert.strictEqual(p.set(o, s), true);
assert.strictEqual(p.has(o), true);
assert.strictEqual(p.get(o), s);
assert.strictEqual(p.set(o, 5), true);
assert.strictEqual(p.has(o), true);
assert.strictEqual(p.get(o), 5);
assert.strictEqual(p.delete(o), true);
assert.strictEqual(p.has(o), false);
assert.strictEqual(p.get(o), undefined);
assert.strictEqual(p.delete(o), false);
assert.throws(() => {
    p.get();
}, TypeError);
assert.throws(() => {
    p.get(11);
}, TypeError);
assert.throws(() => {
    p.has();
}, TypeError);
assert.throws(() => {
    p.has(20);
}, TypeError);
assert.throws(() => {
    p.set();
}, TypeError);
assert.throws(() => {
    p.set(o);
}, TypeError);
assert.throws(() => {
    p.set(9, 17);
}, TypeError);
assert.throws(() => {
    p.delete();
}, TypeError);
assert.throws(() => {
    p.delete(13);
}, TypeError);