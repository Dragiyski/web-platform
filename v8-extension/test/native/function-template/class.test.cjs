const assert = require('node:assert');
const { resolve: resolvePath } = require('node:path');
const native = require(resolvePath(process.env.JS_COMPILED_MODULE_PATH, 'native.node'));

(function () {
    'use strict';

    assert(typeof native.FunctionTemplate === 'function');
    assert.throws(() => {
        native.FunctionTemplate();
    }, TypeError, `FunctionTemplate()`);
    assert.throws(() => {
        new native.FunctionTemplate();
    }, TypeError, `new FunctionTemplate()`);
    assert.throws(() => {
        new native.FunctionTemplate(5);
    }, TypeError, `FunctionTemplate(<Number>)`);
    assert.throws(() => {
        new native.FunctionTemplate('test');
    }, TypeError, `FunctionTemplate(<String>)`);
    assert.throws(() => {
        new native.FunctionTemplate(Symbol('test'));
    }, TypeError, `FunctionTemplate(<Symbol>)`);
    assert.throws(() => {
        new native.FunctionTemplate({});
    }, TypeError, `FunctionTemplate({})`);
    let default_info, default_template, default_function;
    const default_callback = function (info) {
        default_info = info;
    };
    assert.doesNotThrow(() => {
        default_template = new native.FunctionTemplate({
            function: default_callback
        });
    }, `FunctionTemplate({ function })`);
    assert(default_template instanceof native.FunctionTemplate, `default_template instanceof native.FunctionTemplate`);
    assert.doesNotThrow(() => {
        default_function = default_template.get();
    }, `[object FunctionTemplate].get()`);
    assert.strictEqual(typeof default_function, 'function', `typeof default_function === 'function'`);
    assert.doesNotThrow(() => {
        default_function(3, 5, 8);
    });
    assert.strictEqual(default_info, Object(default_info), `typeof default_info === 'object'`);
})();
