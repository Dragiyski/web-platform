#ifndef NODE_EXT_API_OBJECT_TEMPLATE_NAMED_PROPERTY_HANDLER_CONFIGURATION_HXX
#define NODE_EXT_API_OBJECT_TEMPLATE_NAMED_PROPERTY_HANDLER_CONFIGURATION_HXX

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
        v8::Local<v8::Value> get_getter(v8::Isolate *isolate) const;
        v8::Local<v8::Value> get_setter(v8::Isolate *isolate) const;
        v8::Local<v8::Value> get_query(v8::Isolate *isolate) const;
        v8::Local<v8::Value> get_deleter(v8::Isolate *isolate) const;
        v8::Local<v8::Value> get_enumerator(v8::Isolate *isolate) const;
        v8::Local<v8::Value> get_definer(v8::Isolate *isolate) const;
        v8::Local<v8::Value> get_descriptor(v8::Isolate *isolate) const;
    protected:
        NamedPropertyHandlerConfiguration() = default;
        NamedPropertyHandlerConfiguration(const NamedPropertyHandlerConfiguration&) = delete;
        NamedPropertyHandlerConfiguration(NamedPropertyHandlerConfiguration&&) = delete;
    public:
        virtual ~NamedPropertyHandlerConfiguration() override = default;
    };
}

#endif /* NODE_EXT_API_OBJECT_TEMPLATE_NAMED_PROPERTY_HANDLER_CONFIGURATION_HXX */