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
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info);
    private:
        Shared<v8::Function> _getter;
        Shared<v8::Function> _setter;
        v8::PropertyAttribute _attributes;
        v8::AccessControl _access_control;
        v8::SideEffectType _getter_side_effect, _setter_side_effect;
    public:
        v8::Local<v8::Function> get_getter(v8::Isolate* isolate) const;
        v8::Local<v8::Function> get_setter(v8::Isolate* isolate) const;
        v8::PropertyAttribute get_attributes() const;
        v8::AccessControl get_access_control() const;
        v8::SideEffectType get_getter_side_effect() const;
        v8::SideEffectType get_setter_side_effect() const;
    public:
        v8::Maybe<void> setup(v8::Isolate *isolate, v8::Local<v8::Template> target, v8::Local<v8::Name> name, v8::Local<v8::Object> js_template_wrapper) const;
    protected:
        NativeDataProperty(
            v8::Isolate *isolate,
            v8::Local<v8::Function> getter,
            v8::Local<v8::Function> setter,
            v8::PropertyAttribute attributes,
            v8::AccessControl access_control,
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