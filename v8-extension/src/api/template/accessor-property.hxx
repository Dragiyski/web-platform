#ifndef NODE_EXT_API_TEMPLATE_ACCESSOR_PROPERTY_HXX
#define NODE_EXT_API_TEMPLATE_ACCESSOR_PROPERTY_HXX

#include <v8.h>
#include "../template.hxx"
#include "../../js-helper.hxx"
#include "../../wrapper.hxx"

namespace dragiyski::node_ext {
    using namespace js;
    class Template::AccessorProperty : public Wrapper {
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_getter(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_setter(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_attributes(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        Shared<v8::Object> _getter_object;
        Shared<v8::FunctionTemplate> _getter;
        Shared<v8::Object> _setter_object;
        Shared<v8::FunctionTemplate> _setter;
        v8::PropertyAttribute _attributes;
    public:
        v8::Local<v8::Object> get_getter_object(v8::Isolate* isolate) const;
        v8::Local<v8::Object> get_setter_object(v8::Isolate* isolate) const;
        v8::PropertyAttribute get_attributes() const;
    public:
        void apply(v8::Isolate *isolate, v8::Local<v8::Template> receiver);
    protected:
        AccessorProperty(
            v8::Isolate* isolate,
            v8::Local<v8::Object> getter_object,
            v8::Local<v8::FunctionTemplate> getter,
            v8::Local<v8::Object> setter_object,
            v8::Local<v8::FunctionTemplate> setter,
            v8::PropertyAttribute attributes
        );
        AccessorProperty(const AccessorProperty&) = delete;
        AccessorProperty(AccessorProperty&&) = delete;
    public:
        virtual ~AccessorProperty() override = default;
    };
}

#endif /* NODE_EXT_API_TEMPLATE_ACCESSOR_PROPERTY_HXX */