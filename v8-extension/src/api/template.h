#ifndef V8EXT_API_TEMPLATE_H
#define V8EXT_API_TEMPLATE_H

#include "../object.h"
#include <v8.h>
#include "api-helper.h"
#include "../js-helper.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    class Template : public ObjectWrap {
        DECLARE_API_WRAPPER_HEAD(Template)
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
    protected:
    public:
        virtual v8::Local<v8::Template> this_template(v8::Isolate *isolate) = 0;
    protected:
        explicit Template(v8::Isolate *isolate);
        Template(const Template &) = delete;
        Template(Template &&) = delete;
    public:
        ~Template() override = default;
    };
}

#endif /* V8EXT_API_TEMPLATE_H */