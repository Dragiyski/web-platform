#ifndef NODE_EXT_API_TEMPLATE_NATIVE_DATA_PROPERTY_HXX
#define NODE_EXT_API_TEMPLATE_NATIVE_DATA_PROPERTY_HXX

#include <v8.h>
#include "../template.hxx"
#include "../../js-helper.hxx"
#include "../../wrapper.hxx"

namespace dragiyski::node_ext {
    using namespace js;
    class Template::NativeDataProperty : public Wrapper {
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        Shared<v8::Function> _getter;
        Shared<v8::Function> _setter;
        v8::PropertyAttribute _attributes;
        v8::AccessControl _access_control;
        Shared<v8::Object> _signature_object;
        Shared<v8::Signature> _signature;
        v8::SideEffectType _getter_side_effct, _setter_side_effect;
    public:
        v8::Local<v8::Function> get_getter(v8::Isolate* isolate) const;
        v8::Local<v8::Function> get_setter(v8::Isolate* isolate) const;
        v8::PropertyAttribute get_attributes() const;
        v8::AccessControl get_access_control() const;
        v8::Local<v8::Object> get_signature_object(v8::Isolate* isolate) const;
        v8::Local<v8::Signature> get_signature(v8::Isolate *isolate) const;
        v8::SideEffectType get_getter_side_effect() const;
        v8::SideEffectType get_setter_side_effect() const;
    public:
        void apply(v8::Isolate* isolate, v8::Local<v8::Template> receiver);
    protected:
        NativeDataProperty(
            v8::Isolate *isolate,
            v8::Local<v8::Function> getter,
            v8::Local<v8::Function> setter,
            v8::PropertyAttribute attributes,
            v8::AccessControl access_control,
            v8::Local<v8::Object> signature_object,
            v8::Local<v8::Signature> signature,
            v8::SideEffectType getter_side_effect,
            v8::SideEffectType setter_side_effect
        );
        NativeDataProperty(const NativeDataProperty&) = delete;
        NativeDataProperty(NativeDataProperty&&) = delete;
    public:
        virtual ~NativeDataProperty() override = default;
    };
}

#endif /* NODE_EXT_API_TEMPLATE_NATIVE_DATA_PROPERTY_HXX */