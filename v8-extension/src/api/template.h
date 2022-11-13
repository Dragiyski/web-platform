#ifndef V8EXT_TEMPLATE_H
#define V8EXT_TEMPLATE_H

#include <node_object_wrap.h>
#include <v8.h>
#include "api-helper.h"

namespace dragiyski::node_ext {
    class Template : public node::ObjectWrap {
        DECLARE_API_WRAPPER_HEAD
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        virtual v8::Local<v8::Template> value_upcast(v8::Isolate *isolate) = 0;
    public:
        inline v8::Local<v8::Template> value(v8::Isolate *isolate) { return value_upcast(isolate); }
    protected:
        Template() = default;
        Template(const Template &) = delete;
        Template(Template &&) = delete;
    public:
        ~Template() override = default;
    };
}

#endif /* V8EXT_TEMPLATE_H */