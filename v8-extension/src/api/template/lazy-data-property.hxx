#ifndef NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX
#define NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX

#include <v8.h>
#include "../template.hxx"
#include "../../js-helper.hxx"
#include "../../object.hxx"

namespace dragiyski::node_ext {
    using namespace js;
    class Template::LazyDataProperty : public Object<Template::LazyDataProperty> {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        static void getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
    private:
        Shared<v8::Value> _getter;
        v8::PropertyAttribute _attributes;
        v8::SideEffectType _getter_side_effect, _setter_side_effect;
    public:
        v8::Local<v8::Value> get_getter(v8::Isolate* isolate) const;
        v8::PropertyAttribute get_attributes() const;
        v8::SideEffectType get_getter_side_effect() const;
        v8::SideEffectType get_setter_side_effect() const;
    protected:
        LazyDataProperty() = default;
        LazyDataProperty(const LazyDataProperty&) = delete;
        LazyDataProperty(NativeDataProperty&&) = delete;
    public:
        virtual ~LazyDataProperty() override = default;
    };
}

#endif /* NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX */