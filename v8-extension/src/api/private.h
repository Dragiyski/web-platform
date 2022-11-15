#ifndef V8EXT_API_PRIVATE_H
#define V8EXT_API_PRIVATE_H

#include <node_object_wrap.h>
#include <v8.h>
#include "api-helper.h"
#include "../object.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    class Private : public ObjectWrap {
        DECLARE_API_WRAPPER_HEAD(Private)
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_get(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_set(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_has(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_delete(const v8::FunctionCallbackInfo<v8::Value> &info);
    protected:
        Shared<v8::Private> _value;
    public:
        Local<v8::Private> value(v8::Isolate *isolate);
    protected:
        Private(v8::Isolate *isolate, v8::Local<v8::Private> value);
        Private(const Private &) = delete;
        Private(Private &&) = delete;
    public:
        ~Private() override = default;
    };
}

#endif /* V8EXT_API_PRIVATE_H */