#ifndef NODE_EXT_API_FUNCTION_TEMPLATE_HXX
#define NODE_EXT_API_FUNCTION_TEMPLATE_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../wrapper.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    /**
     * @brief Provide access to function template API.
     *
     * @note There won't be any template class or methods for this object.
     * Template API is weird. It contains mostly setters making it write-only (we cannot read what we setup).
     * Many functions are not idempotent. For example accessing PrototypeTemplate() is not just getter, but creates
     * the prototype template the first time it is entered. Any attempt to Inherit() or SetPrototypeProviderTemplate()
     * will crash.
     * Finally, v8::FunctionTemplate inherits from v8::Template which allows setting data properties, accessors and interceptors...
     * All calls to Set* are aggregated into fixed list, even when v8::Name is given.
     * Upon instantiation data properties might throw if duplicate and previous Set is not configurable.
     *
     * So to prevent this case, we fully initialize the FunctionTemplate (and ObjectTemplate) within the constructor.
     * The options object must provide:
     * An object for all named properties that can be:
     * instanceof TemplateAccessor
     * instanceof TemplateLazyDataProperty
     * instanceof TemplateNativeDataProperty
     * instanceof TemplateValue (allow specifying holders of Accessor/Lazy object as a simple value)
     * instanceof Template
     *
     */
    class FunctionTemplate : public Wrapper {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void callback(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        Shared<v8::FunctionTemplate> _value;
        Shared<v8::Function> _callee;
    public:
        v8::Local<v8::FunctionTemplate> value(v8::Isolate *isolate) const;
        v8::Local<v8::Function> callee(v8::Isolate *isolate) const;
    protected:
        FunctionTemplate(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> value, v8::Local<v8::Function> callee);
        FunctionTemplate(const FunctionTemplate&) = delete;
        FunctionTemplate(FunctionTemplate&&) = delete;
    public:
        virtual ~FunctionTemplate() override = default;
    };
}

#endif /* NODE_EXT_API_FUNCTION_TEMPLATE_HXX */