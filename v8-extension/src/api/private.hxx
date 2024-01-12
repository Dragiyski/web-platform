#ifndef NODE_EXT_API_PRIVATE_HXX
#define NODE_EXT_API_PRIVATE_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../object.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    class Private : public virtual Object<Private> {
    public:
        static void initialize(v8::Isolate *isolate);
        static void uninitialize(v8::Isolate *isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate *isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_get(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_set(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_has(const v8::FunctionCallbackInfo<v8::Value> &info);
        static void prototype_delete(const v8::FunctionCallbackInfo<v8::Value> &info);
    private:
        Shared<v8::Private> _value;
    public:
        v8::Local<v8::Private> get_value(v8::Isolate *isolate) const;
    protected:
        Private(v8::Isolate *isolate, v8::Local<v8::Private> value);
        Private(const Private &) = delete;
        Private(Private &&) = delete;
    public:
        virtual ~Private() override = default;
    };
}

#endif /* V8EXT_API_PRIVATE_HXX */