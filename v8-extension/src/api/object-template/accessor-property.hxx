#ifndef NODE_EXT_API_OBJECT_TEMPLATE_ACCESSOR_PROPERTY_HXX
#define NODE_EXT_API_OBJECT_TEMPLATE_ACCESSOR_PROPERTY_HXX

#include <v8.h>
#include "../object-template.hxx"
#include "../../js-helper.hxx"
#include "../../object.hxx"

namespace dragiyski::node_ext {
    using namespace js;
    class ObjectTemplate::AccessorProperty : public Object<ObjectTemplate::AccessorProperty> {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        static void getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info);
        static void prototype_get_getter(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_setter(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_attributes(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_access_control(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_getter_side_effect(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_setter_side_effect(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        Shared<v8::Value> _getter;
        Shared<v8::Value> _setter;
        v8::PropertyAttribute _attributes;
        v8::AccessControl _access_control;
        v8::SideEffectType _getter_side_effect, _setter_side_effect;
    public:
        v8::Local<v8::Value> get_getter(v8::Isolate* isolate) const;
        v8::Local<v8::Value> get_setter(v8::Isolate* isolate) const;
        v8::PropertyAttribute get_attributes() const;
        v8::AccessControl get_access_control() const;
        v8::SideEffectType get_getter_side_effect() const;
        v8::SideEffectType get_setter_side_effect() const;
    protected:
        AccessorProperty() = default;
        AccessorProperty(const AccessorProperty&) = delete;
        AccessorProperty(AccessorProperty&&) = delete;
    public:
        virtual ~AccessorProperty() override = default;
    };
}

#endif /* NODE_EXT_API_OBJECT_TEMPLATE_ACCESSOR_PROPERTY_HXX */