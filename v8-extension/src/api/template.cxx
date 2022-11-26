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
                if (private_value.IsEmpty()) {
                    JS_THROW_ERROR(ReferenceError, isolate, "[object Private] no longer references v8::Private");
                }
                if (value->IsObject()) {
                    auto object = value.As<v8::Object>();
                    auto function_template_holder = object->FindInstanceInPrototypeChain(FunctionTemplate::get_class_template(isolate));
                    if (!function_template_holder.IsEmpty()) {
                        auto function_template_wrapper = Wrapper::Unwrap<FunctionTemplate>(isolate, function_template_holder);
                        if (function_template_wrapper == nullptr) {
                            JS_THROW_ERROR(ReferenceError, isolate, "[object FunctionTemplate] no longer wraps a native object");
                        }
                        auto function_template = function_template_wrapper->get_value(isolate);
                        if (function_template.IsEmpty()) {
                            JS_THROW_ERROR(ReferenceError, isolate, "[object FunctionTemplate] no longer references v8::FunctionTemplate");
                        }
                        target->SetPrivate(private_value, function_template);
                        return v8::JustVoid();
                    }
                    auto object_template_holder = object->FindInstanceInPrototypeChain(ObjectTemplate::get_class_template(isolate));
                    if (!object_template_holder.IsEmpty()) {
                        auto object_template_wrapper = Wrapper::Unwrap<ObjectTemplate>(isolate, object_template_holder);
                        if (object_template_wrapper == nullptr) {
                            JS_THROW_ERROR(ReferenceError, isolate, "[object ObjectTemplate] no longer wraps a native object");
                        }
                        auto object_template = object_template_wrapper->get_value(isolate);
                        if (object_template.IsEmpty()) {
                            JS_THROW_ERROR(ReferenceError, isolate, "[object ObjectTemplate] no longer references v8::ObjectTemplate");
                        }
                        target->SetPrivate(private_value, object_template);
                        return v8::JustVoid();
                    }
                    JS_THROW_ERROR(TypeError, isolate, "Template::SetPrivate can only be set to [object FunctionTemplate], [object ObjectTemplate] or primitive value");
                } else if (value->IsExternal()) {
                    JS_THROW_ERROR(TypeError, isolate, "Template::SetPrivate can only be set to [object FunctionTemplate], [object ObjectTemplate] or primitive value");
                } else {
                    // Primitive value:
                    target->SetPrivate(private_value, value);
                }
            } else if (name->IsString() || name->IsSymbol()) {
                if (value->IsObject()) {

                }
            }
        }
        v8::Maybe<void> ConfigureFromMap(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Map> properties) {
            static constexpr const auto __function_return_type__ = v8::Nothing<void>;
            auto isolate = context->GetIsolate();

            auto entries = properties->AsArray();
            for (decltype(entries->Length()) i = 0; i < entries->Length(); i += 2) {
                JS_EXPRESSION_RETURN(name, entries->Get(context, i));
                JS_EXPRESSION_RETURN(value, entries->Get(context, i + 1));
                JS_EXPRESSION_IGNORE(ConfigureEntry(context, target, name, value));
            }
        }
        v8::Maybe<void> ConfigureFromObject(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Object> properties) {
            static constexpr const auto __function_return_type__ = v8::Nothing<void>;
            auto isolate = context->GetIsolate();

            JS_EXPRESSION_RETURN(names, properties->GetPropertyNames(context));
            for (decltype(names->Length()) i = 0; i < names->Length(); ++i) {
                JS_EXPRESSION_RETURN(name, names->Get(context, i));
                JS_EXPRESSION_RETURN(value, properties->Get(name, i));
                JS_EXPRESSION_IGNORE(ConfigureEntry(context, target, name, value));
            }
        }
    }

    v8::Maybe<void> Template::ConfigureTemplate(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Value> properties) {
        static constexpr const auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        if (properties->IsMap()) {
            JS_EXPRESSION_IGNORE(ConfigureFromMap(context, target, properties.As<v8::Map>()));
        } else if (properties->IsObject()) {
            JS_EXPRESSION_IGNORE(ConfigureFromObject(context, target, properties.As<v8::Object>()));
        } else {
            JS_THROW_ERROR(TypeError, isolate, 'Cannot convert value to [object Map] or [object Object].');
        }
        return v8::JustVoid();
    }
}
