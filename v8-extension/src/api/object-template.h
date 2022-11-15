#ifndef V8EXT_OBJECT_TEMPLATE_H
#define V8EXT_OBJECT_TEMPLATE_H

#include <node_object_wrap.h>
#include <v8.h>
#include "api-helper.h"
#include "../js-helper.h"
#include "template.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    class ObjectTemplate : public Template {
        DECLARE_API_WRAPPER_HEAD(ObjectTemplate)
    public:
        static MaybeLocal<v8::Object> New(Local<v8::Context>, Local<v8::ObjectTemplate>);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void create(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        Shared<v8::ObjectTemplate> _value;
    public:
        Local<v8::ObjectTemplate> this_object_template(v8::Isolate *isolate);
        Local<v8::Template> this_template(v8::Isolate *isolate) override;
    protected:
        ObjectTemplate(v8::Isolate *isolate, Local<v8::ObjectTemplate> value);
        ObjectTemplate(const ObjectTemplate &) = delete;
        ObjectTemplate(ObjectTemplate &&) = delete;
    public:
        ~ObjectTemplate() override = default;
    };
}

#endif /* V8EXT_OBJECT_TEMPLATE_H */