#ifndef V8EXT_OBJECT_TEMPLATE_H
#define V8EXT_OBJECT_TEMPLATE_H

#include <node_object_wrap.h>
#include <v8.h>
#include "api-helper.h"
#include "template.h"

namespace dragiyski::node_ext {
    class ObjectTemplate : public Template {
        DECLARE_API_WRAPPER_HEAD
    public:
        static v8::MaybeLocal<v8::Object> New(v8::Local<v8::Context>, v8::Local<v8::ObjectTemplate>);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        v8::Global<v8::ObjectTemplate> _value;
        inline v8::Local<v8::Template> value_upcast(v8::Isolate *isolate) { return value(isolate); }
    public:
        v8::Local<v8::ObjectTemplate> value(v8::Isolate *isolate);
    protected:
        ObjectTemplate(v8::Isolate *isolate, v8::Local<v8::ObjectTemplate> value);
        ObjectTemplate(const ObjectTemplate &) = delete;
        ObjectTemplate(ObjectTemplate &&) = delete;
    public:
        ~ObjectTemplate() override = default;
    };
}

#endif /* V8EXT_OBJECT_TEMPLATE_H */