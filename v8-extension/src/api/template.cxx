#include "template.hxx"

#include "private.hxx"
#include "function-template.hxx"
#include "object-template.hxx"
#include "template/accessor-property.hxx"
#include "template/native-data-property.hxx"
#include "template/lazy-data-property.hxx"

#include "../js-string-table.hxx"
#include "../error-message.hxx"
#include <map>

namespace dragiyski::node_ext {
    using namespace js;

    namespace {
        v8::Maybe<void> ConfigureEntry(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Value> name, v8::Local<v8::Value> value) {
            static constexpr const auto __function_return_type__ = v8::Nothing<void>;
            auto isolate = context->GetIsolate();
            v8::HandleScope scope(isolate);

            if (name->IsObject()) {
                // Only executed when the property value was map. The only objects accepted as key are the [object Private] instances.
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(
                    private_wrapper,
                    Wrapper::Unwrap<Private>(isolate, name.As<v8::Object>(), Private::get_class_template(isolate), "Private"),
                    context,
                    "Invalid key type, expected object instance of \"Private\""
                )
                auto private_value = private_wrapper->get_value(isolate);
                if V8_UNLIKELY(private_value.IsEmpty()) {
                    JS_THROW_ERROR(ReferenceError, isolate, "[object Private] no longer references v8::Private");
                }
                // For now the class hierarchy of the v8::Value is one of:
                // - Object
                // - External
                // - Primitive
                if (value->IsObject()) {
                    {
                        JS_EXPRESSION_RETURN(wrapper, Wrapper::TryUnwrap<FunctionTemplate>(isolate, value.As<v8::Object>(), FunctionTemplate::get_class_template(isolate)));
                        if V8_UNLIKELY(wrapper != nullptr) {
                            auto js_value = wrapper->get_value(isolate);
                            if V8_UNLIKELY(js_value.IsEmpty()) {
                                JS_THROW_ERROR(ReferenceError, isolate, "[object FunctionTemplate] no longer references v8::FunctionTemplate");
                            }
                            target->SetPrivate(private_value, js_value);
                            return v8::JustVoid();
                        }
                    }
                    {
                        JS_EXPRESSION_RETURN(wrapper, Wrapper::TryUnwrap<ObjectTemplate>(isolate, value.As<v8::Object>(), ObjectTemplate::get_class_template(isolate)));
                        if V8_UNLIKELY(wrapper != nullptr) {
                            auto js_value = wrapper->get_value(isolate);
                            if V8_UNLIKELY(js_value.IsEmpty()) {
                                JS_THROW_ERROR(ReferenceError, isolate, "[object ObjectTemplate] no longer references v8::FunctionTemplate");
                            }
                            target->SetPrivate(private_value, js_value);
                            return v8::JustVoid();
                        }
                    }
                    JS_THROW_ERROR(TypeError, isolate, "Template::SetPrivate can only be set to [object FunctionTemplate], [object ObjectTemplate] or primitive value");
                } else if V8_UNLIKELY(value->IsExternal()) {
                    JS_THROW_ERROR(TypeError, isolate, "Template::SetPrivate can only be set to [object FunctionTemplate], [object ObjectTemplate] or primitive value");
                } else /* v8::Primitive */ {
                    target->SetPrivate(private_value, value);
                }
            } else if (name->IsString() || name->IsSymbol()) {
                // Accepted values are the same as above + additional helper objects:
                // A primitive, FunctionTemplate or ObjectTemplate object wrapper will call Set() to set a property;
                // An AccessorProperty, LazyDataProperty or NativeDataProperty object that wraps a value for the property, calls corrensponding Set* method.
                if (value->IsObject()) {
                    {
                        JS_EXPRESSION_RETURN(wrapper, Wrapper::TryUnwrap<FunctionTemplate>(isolate, value.As<v8::Object>(), FunctionTemplate::get_class_template(isolate)));
                        if V8_LIKELY(wrapper != nullptr) {
                            auto js_value = wrapper->get_value(isolate);
                            if V8_UNLIKELY(js_value.IsEmpty()) {
                                JS_THROW_ERROR(ReferenceError, isolate, "[object FunctionTemplate] no longer references v8::FunctionTemplate");
                            }
                            target->Set(name.As<v8::Name>(), js_value);
                            return v8::JustVoid();
                        }
                    }
                    {
                        JS_EXPRESSION_RETURN(wrapper, Wrapper::TryUnwrap<ObjectTemplate>(isolate, value.As<v8::Object>(), ObjectTemplate::get_class_template(isolate)));
                        if V8_LIKELY(wrapper != nullptr) {
                            auto js_value = wrapper->get_value(isolate);
                            if V8_UNLIKELY(js_value.IsEmpty()) {
                                JS_THROW_ERROR(ReferenceError, isolate, "[object ObjectTemplate] no longer references v8::FunctionTemplate");
                            }
                            target->Set(name.As<v8::Name>(), js_value);
                            return v8::JustVoid();
                        }
                    }
                    {
                        JS_EXPRESSION_RETURN(wrapper, Wrapper::TryUnwrap<Template::NativeDataProperty>(isolate, value.As<v8::Object>(), Template::NativeDataProperty::get_class_template(isolate)));
                        if V8_LIKELY(wrapper != nullptr) {
                            JS_EXPRESSION_IGNORE(wrapper->setup(isolate, target, name.As<v8::Name>(), value.As<v8::Object>()));
                            return v8::JustVoid();
                        }
                    }
                    {
                        JS_EXPRESSION_RETURN(wrapper, Wrapper::TryUnwrap<Template::LazyDataProperty>(isolate, value.As<v8::Object>(), Template::LazyDataProperty::get_class_template(isolate)));
                        if V8_LIKELY(wrapper != nullptr) {
                            JS_EXPRESSION_IGNORE(wrapper->setup(isolate, target, name.As<v8::Name>(), value.As<v8::Object>()));
                            return v8::JustVoid();
                        }
                    }
                    {
                        JS_EXPRESSION_RETURN(wrapper, Wrapper::TryUnwrap<Template::AccessorProperty>(isolate, value.As<v8::Object>(), Template::AccessorProperty::get_class_template(isolate)));
                        if V8_LIKELY(wrapper != nullptr) {
                            JS_EXPRESSION_IGNORE(wrapper->setup(isolate, target, name.As<v8::Name>(), value.As<v8::Object>()));
                            return v8::JustVoid();
                        }
                    }
                }
            }
        }
        v8::Maybe<void> ConfigureFromMap(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Map> properties) {
            static const constexpr auto __function_return_type__ = v8::Nothing<void>;

            auto entries = properties->AsArray();
            for (decltype(entries->Length()) i = 0; i < entries->Length(); i += 2) {
                JS_EXPRESSION_RETURN(name, entries->Get(context, i));
                JS_EXPRESSION_RETURN(value, entries->Get(context, i + 1));
                JS_EXPRESSION_IGNORE(ConfigureEntry(context, target, name, value));
            }
            return v8::JustVoid();
        }
        v8::Maybe<void> ConfigureFromObject(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Object> properties) {
            static const constexpr auto __function_return_type__ = v8::Nothing<void>;

            JS_EXPRESSION_RETURN(names, properties->GetPropertyNames(context));
            for (decltype(names->Length()) i = 0; i < names->Length(); ++i) {
                JS_EXPRESSION_RETURN(name, names->Get(context, i));
                JS_EXPRESSION_RETURN(value, properties->Get(context, name));
                JS_EXPRESSION_IGNORE(ConfigureEntry(context, target, name, value));
            }
            return v8::JustVoid();
        }
    }

    v8::Maybe<void> Template::ConfigureTemplate(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Value> properties) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        if (properties->IsMap()) {
            JS_EXPRESSION_IGNORE(ConfigureFromMap(context, target, properties.As<v8::Map>()));
            return v8::JustVoid();
        }
        if (properties->IsObject()) {
            JS_EXPRESSION_IGNORE(ConfigureFromObject(context, target, properties.As<v8::Object>()));
            return v8::JustVoid();
        }
        JS_THROW_ERROR(TypeError, isolate, "Cannot convert value to [object Map] or [object Object].");
    }
}
