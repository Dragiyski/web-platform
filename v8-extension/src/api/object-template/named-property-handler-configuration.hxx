#ifndef NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX
#define NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX

#include <v8.h>
#include "../object-template.hxx"
#include "../../js-helper.hxx"
#include "../../object.hxx"

namespace dragiyski::node_ext {
    using namespace js;
    class ObjectTemplate::NamedPropertyHandlerConfiguration : public Object<ObjectTemplate::NamedPropertyHandlerConfiguration> {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        v8::PropertyHandlerFlags _flags;
        Shared<v8::Value> _getter, _setter, _query, _deleter, _enumerator, _definer, _descriptor;
    public:
        const v8::PropertyHandlerFlags &get_flags() const;
        const v8::Local<v8::Value> &get_getter(v8::Isolate *isolate) const;
        const v8::Local<v8::Value> &get_setter(v8::Isolate *isolate) const;
        const v8::Local<v8::Value> &get_query(v8::Isolate *isolate) const;
        const v8::Local<v8::Value> &get_deleter(v8::Isolate *isolate) const;
        const v8::Local<v8::Value> &get_enumerator(v8::Isolate *isolate) const;
        const v8::Local<v8::Value> &get_definer(v8::Isolate *isolate) const;
        const v8::Local<v8::Value> &get_descriptor(v8::Isolate *isolate) const;
    protected:
        static void NamedPropertyGetterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void NamedPropertySetterCallback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void NamedPropertyQueryCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Integer> &info);
        static void NamedPropertyDeleterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Boolean> &info);
        static void NamedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array> &info);
        static void NamedPropertyDefinerCallback(v8::Local<v8::Name> property, const v8::PropertyDescriptor &descriptor, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void NamedPropertyDescriptorCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
    protected:
        NamedPropertyHandlerConfiguration() = default;
        NamedPropertyHandlerConfiguration(const NamedPropertyHandlerConfiguration&) = delete;
        NamedPropertyHandlerConfiguration(NamedPropertyHandlerConfiguration&&) = delete;
    public:
        virtual ~NamedPropertyHandlerConfiguration() override = default;
    };
}

#endif /* NODE_EXT_API_TEMPLATE_LAZY_DATA_PROPERTY_HXX */