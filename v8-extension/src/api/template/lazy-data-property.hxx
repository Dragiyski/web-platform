#ifndef NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX
#define NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX

#include <v8.h>
#include "../template.hxx"
#include "../../js-helper.hxx"
#include "../../wrapper.hxx"

namespace dragiyski::node_ext {
    using namespace js;
    class Template::LazyDataProperty : public Wrapper {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
    private:
        Shared<v8::Function> _getter;
        Shared<v8::Function> _setter;
        v8::PropertyAttribute _attributes;
        Shared<v8::Object> _signature_object;
        Shared<v8::Signature> _signature;
        v8::SideEffectType _getter_side_effect, _setter_side_effect;
    public:
        v8::Local<v8::Function> get_getter(v8::Isolate* isolate) const;
        v8::PropertyAttribute get_attributes() const;
        v8::SideEffectType get_getter_side_effect() const;
        v8::SideEffectType get_setter_side_effect() const;
    public:
        v8::Maybe<void> setup(v8::Isolate *isolate, v8::Local<v8::Template> target, v8::Local<v8::Name> name, v8::Local<v8::Object> js_template_wrapper) const;
    protected:
        LazyDataProperty(
            v8::Isolate *isolate,
            v8::Local<v8::Function> getter,
            v8::PropertyAttribute attributes,
            v8::SideEffectType getter_side_effect,
            v8::SideEffectType setter_side_effect
        );
        LazyDataProperty(const LazyDataProperty&) = delete;
        LazyDataProperty(NativeDataProperty&&) = delete;
    public:
        virtual ~LazyDataProperty() override = default;
    };
}

#endif /* NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX */