#ifndef V8EXT_FUNCTION_TEMPLATE_H
#define V8EXT_FUNCTION_TEMPLATE_H

#include <node_object_wrap.h>
#include <v8.h>
#include "api-helper.h"

namespace dragiyski::node_ext {
    class FunctionTemplate : public node::ObjectWrap {
        DECLARE_API_WRAPPER_HEAD
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void invoke(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        v8::Persistent<v8::FunctionTemplate> _value;
    public:
        v8::Local<v8::FunctionTemplate> value(v8::Isolate *isolate);
    protected:
        FunctionTemplate(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> value);
        FunctionTemplate(const FunctionTemplate &) = delete;
        FunctionTemplate(FunctionTemplate &&) = delete;
    public:
        ~FunctionTemplate() override = default;
    };
}

#endif /* V8EXT_FUNCTION_TEMPLATE_H */