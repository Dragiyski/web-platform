#ifndef NODE_EXT_API_TEMPLATE_HXX
#define NODE_EXT_API_TEMPLATE_HXX

#include <concepts>
#include <v8.h>
#include "../js-helper.hxx"
#include "../object.hxx"
#include "frozen-map.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    /**
     * @brief Template is not a wrapper. The constructor must throw `TypeError("Illegal constructor");`
     * 
     * This is used as a base class in JavaScript for both FunctionTemplate and ObjectTemplate wrappers.
     * 
     * The entire ObjectTemplate/FunctionTemplate must be initialized in the constructor.
     * The constructor will accept the following:
     * 
     * ```javascript
     * new ObjectConstructor({
     *     properties: {
     *         <name/symbol>: <Template.AccessorProperty/Template.NativeDataProperty/Template.LazyDataProperty/ObjectTemplate.Accessor/FunctionTemplate/ObjectTemplate/primitive
     *     }
     * })
     * ```
     * 
     * properties must be either Object or a Map. If Object, only own enumerable properties and symbols are examined.
     * The value must be either primitive or an object instanceof one the classes specified above.
     * 
     * If properties is a Map, keys must be either a string, a Symbol or an Object instanceof Private.
     * Alternatively is the option "private" is a Map, it must contain only keys that are objects instanceof Private.
     * The values for Privates can only be FunctionTemplate/ObjectTemplate or primitive values (including symbols).
     * 
     * The above specification does not allow repetition of keys. Objects cannot have duplicate string key or symbol. Maps cannot specify
     * the same primitive or object twice.
     * 
     * Wrappers of those objects must be stored in a map in the template wrapper, so they can be examined. However, all values will be
     * readonly.
     */
    class Template {
    public:
        static void initialize(v8::Isolate *isolate);
        static void uninitialize(v8::Isolate *isolate);
    public:
        static v8::Local<v8::Private> get_template_symbol(v8::Isolate *isolate);
    public:
        template <typename Type>
        static v8::Maybe<void> Setup(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<typename Type::js_type> target, v8::Local<v8::Map> map, v8::Local<v8::Value> properties);
        static v8::Maybe<void> SetupProperty(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::Template> target, v8::Local<v8::Map> map, v8::Local<v8::Value> key, v8::Local<v8::Value> value);
    private:
        template <typename Type>
        static v8::Maybe<void> SetupFromMap(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<typename Type::js_type> target, v8::Local<v8::Map> map, v8::Local<v8::Map> properties);
        template <typename Type>
        static v8::Maybe<void> SetupFromObject(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<typename Type::js_type> target, v8::Local<v8::Map> map, v8::Local<v8::Object> properties);
    public:
        class NativeDataProperty;
        class LazyDataProperty;
    };

    template<class Type>
    inline v8::Maybe<void> Template::Setup(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<typename Type::js_type> target, v8::Local<v8::Map> map, v8::Local<v8::Value> source) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        if (source->IsMap()) {
            JS_EXPRESSION_IGNORE(SetupFromMap<Type>(context, target, map, source.As<v8::Map>()));
        } else if (source->IsObject()) {
            auto frozen_map = Object<FrozenMap>::get_implementation(isolate, source.As<v8::Object>());
            if (frozen_map != nullptr) {
                auto mutable_map = frozen_map->get_map(isolate);
                JS_EXPRESSION_IGNORE(SetupFromMap<Type>(context, interface, target, map, mutable_map));
            } else {
                JS_EXPRESSION_IGNORE(SetupFromObject<Type>(context, interface, target, map, source.As<v8::Object>()));
            }
        } else {
            JS_THROW_ERROR(TypeError, isolate, "Cannot convert value to [object Map], [object FrozenMap], or [object Object].");
        }
        return v8::JustVoid();
    }

    template<class Type>
    inline v8::Maybe<void> Template::SetupFromMap(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<typename Type::js_type> target, v8::Local<v8::Map> map, v8::Local<v8::Map> source) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        auto key_value = source->AsArray();
        for (decltype(key_value->Length()) i = 0; i < key_value->Length(); i += 2) {
            JS_EXPRESSION_RETURN(key, key_value->Get(context, i + 0));
            JS_EXPRESSION_RETURN(value, key_value->Get(context, i + 1));
            JS_EXPRESSION_IGNORE(Type::SetupProperty(context, interface, target, map, key, value));
        }

        return v8::JustVoid();
    }

    template<class Type>
    inline v8::Maybe<void> Template::SetupFromObject(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<typename Type::js_type> target, v8::Local<v8::Map> map, v8::Local<v8::Object> source) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        JS_EXPRESSION_RETURN(keys, source->GetOwnPropertyNames(context));
        for (decltype(keys->Length()) i = 0; i < keys->Length(); i++) {
            JS_EXPRESSION_RETURN(key, keys->Get(context, i));
            JS_EXPRESSION_RETURN(value, source->Get(context, key));
            JS_EXPRESSION_IGNORE(Type::SetupProperty(context, interface, target, map, key, value));
        }

        return v8::JustVoid();
    }
}

#endif /* NODE_EXT_API_TEMPLATE_HXX */
