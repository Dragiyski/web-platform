#ifndef NODE_EXT_API_OBJECT_TEMPLATE_HXX
#define NODE_EXT_API_OBJECT_TEMPLATE_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../wrapper.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    class ObjectTemplate : public Wrapper {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        Shared<v8::ObjectTemplate> _value;
    public:
        v8::Local<v8::ObjectTemplate> get_value(v8::Isolate *isolate) const;
    protected:
        ObjectTemplate(v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> value);
        ObjectTemplate(const ObjectTemplate&) = delete;
        ObjectTemplate(ObjectTemplate&&) = delete;
    public:
        virtual ~ObjectTemplate() override = default;
    };
}

#endif /* NODE_EXT_API_OBJECT_TEMPLATE_HXX */
