#ifndef NODE_EXT_API_OBJECT_TEMPLATE_HXX
#define NODE_EXT_API_OBJECT_TEMPLATE_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../object.hxx"

namespace dragiyski::node_ext {
    class FunctionTemplate;
}

#include "function-template.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    class ObjectTemplate : public Object<ObjectTemplate> {
        friend class FunctionTemplate;
    public:
        using js_type = v8::ObjectTemplate;
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
        static v8::Maybe<void> ConfigureTemplate(v8::Local<v8::Context> context, v8::Local<v8::ObjectTemplate> value, v8::Local<v8::Object> settings);
    public:
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate* isolate);
    public:
        static v8::Maybe<void> Setup(v8::Local<v8::Context> context, ObjectTemplate* target, v8::Local<v8::Object> options);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        static void NamedPropertyGetterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void NamedPropertySetterCallback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void NamedPropertyQueryCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Integer> &info);
        static void NamedPropertyDeleterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Boolean> &info);
        static void NamedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array> &info);
        static void NamedPropertyDefinerCallback(v8::Local<v8::Name> property, const v8::PropertyDescriptor &descriptor, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void NamedPropertyDescriptorCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void IndexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void IndexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void IndexedPropertyQueryCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Integer> &info);
        static void IndexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean> &info);
        static void IndexedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array> &info);
        static void IndexedPropertyDefinerCallback(uint32_t index, const v8::PropertyDescriptor &descriptor, const v8::PropertyCallbackInfo<v8::Value> &info);
        static void IndexedPropertyDescriptorCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value> &info);
    private:
        Shared<v8::ObjectTemplate> _value;
        bool _undetectable;
        bool _code_like;
        bool _immutable_prototype;
        Shared<v8::Object> _name_handler, _index_handler;
    public:
        v8::Local<v8::ObjectTemplate> get_value(v8::Isolate *isolate) const;
    protected:
        ObjectTemplate() = default;
        ObjectTemplate(const ObjectTemplate&) = delete;
        ObjectTemplate(ObjectTemplate&&) = delete;
    public:
        virtual ~ObjectTemplate() override = default;
    public:
        class NamedPropertyHandlerConfiguration;
        class IndexedPropertyHandlerConfiguration;
    };
}

#endif /* NODE_EXT_API_OBJECT_TEMPLATE_HXX */
