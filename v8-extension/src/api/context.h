#ifndef V8EXT_API_CONTEXT_H
#define V8EXT_API_CONTEXT_H

#include <memory>
#include <v8.h>
#include "api-helper.h"
#include "../object.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    class Context : public ObjectWrap {
        DECLARE_API_WRAPPER_HEAD(Context);
        static Maybe<void> initialize(Local<v8::Context> context);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void for_object(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void compile_function(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        static MaybeLocal<v8::Context> GetCreationContext(Local<v8::Context> context, Local<v8::Object> object);
        static MaybeLocal<v8::Object> ForObject(Local<v8::Context> context, Local<v8::Object> object);
    protected:
        Shared<v8::Context> _value;
    public:
        Local<v8::Context> value(v8::Isolate* isolate) const;
    protected:
        Context(v8::Isolate* isolate, Local<v8::Context> context);
        Context(const Context&) = delete;
        Context(Context&&) = delete;
    public:
        ~Context() override = default;
    };
}

#endif /* V8EXT_API_CONTEXT_H */