#ifndef V8EXT_FUNCTION_TEMPLATE_H
#define V8EXT_FUNCTION_TEMPLATE_H

#include <v8.h>
#include "api-helper.h"
#include "template.h"

namespace dragiyski::node_ext {
    class FunctionTemplate : public Template {
        DECLARE_API_WRAPPER_HEAD
        static v8::Maybe<void> initialize_more(v8::Isolate *isolate);
        static void uninitialize_more(v8::Isolate *isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void invoke(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void static_open(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void create(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void default_function(const v8::FunctionCallbackInfo<v8::Value> &info);
    protected:
        v8::Persistent<v8::FunctionTemplate> _value;
        v8::Persistent<v8::Function> _callee;
        inline v8::Local<v8::Template> value_upcast(v8::Isolate *isolate) { return value(isolate); }
    public:
        static v8::Maybe<FunctionTemplate *> unwrap(v8::Isolate *isolate, v8::Local<v8::Object> object);
        v8::Local<v8::FunctionTemplate> value(v8::Isolate *isolate);
        v8::Local<v8::Function> callee(v8::Isolate *isolate);
    protected:
        FunctionTemplate(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> value, v8::Local<v8::Function> callee);
        FunctionTemplate(const FunctionTemplate &) = delete;
        FunctionTemplate(FunctionTemplate &&) = delete;
    public:
        ~FunctionTemplate() override = default;
    };
}

#endif /* V8EXT_FUNCTION_TEMPLATE_H */