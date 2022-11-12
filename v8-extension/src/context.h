#ifndef V8EXT_CONTEXT_H
#define V8EXT_CONTEXT_H

#include <v8.h>

namespace dragiyski::node_ext {
    void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info);
    void js_create_context(const v8::FunctionCallbackInfo<v8::Value> &info);
    v8::Maybe<void> context_init(v8::Local<v8::Context> context);
    v8::Local<v8::FunctionTemplate> context_template(v8::Isolate *isolate);
    v8::Local<v8::Private> context_symbol(v8::Isolate *isolate);
}

#endif /* V8EXT_CONTEXT_H */
