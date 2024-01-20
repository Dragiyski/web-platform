#ifndef NODE_EXT_API_FUNCTION_TEMPLATE_HXX
#define NODE_EXT_API_FUNCTION_TEMPLATE_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../object.hxx"

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
    class FunctionTemplate : public Object<FunctionTemplate> {
    public:
        using js_type = v8::FunctionTemplate;
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_template_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void callback(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        static v8::Maybe<FunctionTemplate *> Create(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::Object> options);
        static v8::Maybe<void> SetupProperty(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::FunctionTemplate> target, v8::Local<v8::Map> map, v8::Local<v8::Value> key, v8::Local<v8::Value> value);
    private:
        Shared<v8::FunctionTemplate> _value;
        Shared<v8::Value> _callee;
        Shared<v8::Object> _receiver, _prototype_provider, _prototype_template, _instance_template, _inherit, _properties;
        bool _accept_any_receiver;
        bool _remove_prototype;
        bool _readonly_prototype;
        bool _allow_construct;
        v8::SideEffectType _side_effect_type;
        int _length; 
        Shared<v8::String> _class_name; 
    public:
        v8::Local<v8::FunctionTemplate> get_value(v8::Isolate *isolate) const;
        v8::Local<v8::Value> get_callee(v8::Isolate *isolate) const;
    protected:
        FunctionTemplate() = default;
        FunctionTemplate(const FunctionTemplate&) = delete;
        FunctionTemplate(FunctionTemplate&&) = delete;
    public:
        virtual ~FunctionTemplate() override = default;
    };
}

#endif /* NODE_EXT_API_FUNCTION_TEMPLATE_HXX */