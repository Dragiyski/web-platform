#ifndef NODE_EXT_API_CONTEXT_HXX
#define NODE_EXT_API_CONTEXT_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../wrapper.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    class Context : public Wrapper {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void static_get_current(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void static_for(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_global(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_compile_function(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        Shared<v8::Context> _value;
    public:
        v8::Local<v8::Context> get_value(v8::Isolate *isolate) const;
    protected:
        Context(v8::Isolate* isolate, v8::Local<v8::Context> value);
        Context(const Context&) = delete;
        Context(Context&&) = delete;
    public:
        virtual ~Context() override = default;
    };
}

#endif /* NODE_EXT_API_CONTEXT_HXX */