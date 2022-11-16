#ifndef V8EXT_API_FUNCTION_TEMPLATE_H
#define V8EXT_API_FUNCTION_TEMPLATE_H

#include <v8.h>
#include "api-helper.h"
#include "template.h"

namespace dragiyski::node_ext {
    class FunctionTemplate : public Template {
        DECLARE_API_WRAPPER_HEAD(FunctionTemplate);
        static Maybe<void> initialize_more(v8::Isolate *isolate);
        static void uninitialize_more(v8::Isolate *isolate);
    public:
        static void invoke(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void default_function(const v8::FunctionCallbackInfo<v8::Value> &info);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void static_open(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void create(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void get_instance_template(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void get_prototype_template(const v8::FunctionCallbackInfo<v8::Value> &info);
    protected:
        Shared<v8::FunctionTemplate> _template;
        Shared<v8::Object> _instance_template_holder;
        Shared<v8::Object> _prototype_template_holder;
        Shared<v8::Function> _callee;
    public:
        Local<v8::FunctionTemplate> this_function_template(v8::Isolate *isolate);
        Local<v8::Template> this_template(v8::Isolate *isolate) override;
        Local<v8::Object> this_instance_template_holder(v8::Isolate *isolate);
        Local<v8::Object> this_prototype_template_holder(v8::Isolate *isolate);
        Local<v8::Function> callee(v8::Isolate *isolate);
    protected:
        FunctionTemplate(
            v8::Isolate *isolate,
            Local<v8::FunctionTemplate> api_template,
            v8::Local<v8::Object> instance_template,
            v8::Local<v8::Object> prototype_template,
            Local<v8::Function> callee
        );
        FunctionTemplate(const FunctionTemplate &) = delete;
        FunctionTemplate(FunctionTemplate &&) = delete;
    public:
        ~FunctionTemplate() override = default;
    };
}

#endif /* V8EXT_API_FUNCTION_TEMPLATE_H */