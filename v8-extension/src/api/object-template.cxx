#include "object-template.hxx"

#include "object-template/accessor-property.hxx"
#include "object-template/named-property-handler-configuration.hxx"
#include "object-template/indexed-property-handler-configuration.hxx"
#include "template.hxx"
#include "frozen-map.hxx"

#include "../error-message.hxx"
#include "../js-string-table.hxx"

#include <map>
#include <vector>

namespace dragiyski::node_ext {
    using namespace js;

    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_template;
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void ObjectTemplate::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = ::js::StringTable::Get(isolate, "FunctionTemplate");
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
        class_template->SetClassName(class_name);

        auto signature = v8::Signature::New(isolate, class_template);
        auto prototype_template = class_template->PrototypeTemplate();

        class_template->ReadOnlyPrototype();
        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_class_symbol.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_cache)
        );

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void ObjectTemplate::uninitialize(v8::Isolate* isolate) {
        per_isolate_template.erase(isolate);
        per_isolate_class_symbol.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> ObjectTemplate::get_template(v8::Isolate* isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    v8::Maybe<ObjectTemplate *> ObjectTemplate::Create(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::Object> options) {
        static const constexpr auto __function_return_type__ = v8::Nothing<ObjectTemplate *>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        v8::Local<v8::ObjectTemplate> target;
        v8::Local<v8::FunctionTemplate> target_constructor;
        {
            auto name = StringTable::Get(isolate, "constructor");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN(attributes, options->GetPropertyAttributes(context, name));
                if (!(attributes & static_cast<uint>(v8::PropertyAttribute::DontEnum))) {
                    if V8_UNLIKELY(!js_value->IsObject()) {
                        JS_THROW_ERROR(TypeError, isolate, "Option \"constructor\" is not an [object FunctionTemplate]");
                    }
                    auto js_object = js_value.As<v8::Object>();
                    auto implementation = Object<FunctionTemplate>::get_implementation(isolate, js_object);
                    if (implementation == nullptr) {
                        JS_THROW_ERROR(TypeError, isolate, "Option \"constructor\" is not an [object FunctionTemplate]");
                    }
                    target_constructor = implementation->get_value(isolate);
                }
            }
        }

        if (!target_constructor.IsEmpty()) {
            target = v8::ObjectTemplate::New(isolate, target_constructor);
        } else {
            target = v8::ObjectTemplate::New(isolate);
        }

        return Create(context, interface, target, options);
    }

    v8::Maybe<ObjectTemplate *> ObjectTemplate::Create(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::ObjectTemplate> js_target, v8::Local<v8::Object> options) {
        static const constexpr auto __function_return_type__ = v8::Nothing<ObjectTemplate *>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        // Usage of unique_ptr here, so in case function returns early (due to error) this is destroyed.
        // We (should) call .release() at the end of this function.
        auto target = std::unique_ptr<ObjectTemplate>(new ObjectTemplate());
        target->set_interface(isolate, interface);
        target->_value.Reset(isolate, js_target);

        target->_undetectable = false;
        {
            auto name = StringTable::Get(isolate, "undetectable");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                target->_undetectable = js_value->BooleanValue(isolate);
                if (target->_undetectable) {
                    js_target->MarkAsUndetectable();
                }
            }
        }

        target->_code_like = false;
        {
            auto name = StringTable::Get(isolate, "codeLike");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                target->_code_like = js_value->BooleanValue(isolate);
                if (target->_code_like) {
                        js_target->SetCodeLike();
                }
            }
        }

        target->_immutable_prototype = false;
        {
            auto name = StringTable::Get(isolate, "immutablePrototype");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                target->_immutable_prototype = js_value->BooleanValue(isolate);
                if (target->_immutable_prototype) {
                    js_target->SetImmutableProto();
                }
            }
        }

        {
            auto name = StringTable::Get(isolate, "namedHandler");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"namedHandler\" is not an [object NamedPropertyHandlerConfiguration]");
                }
                auto js_object = js_value.As<v8::Object>();
                auto named_handler = Object<NamedPropertyHandlerConfiguration>::get_implementation(isolate, js_object);
                if (named_handler == nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"namedHandler\" is not an [object NamedPropertyHandlerConfiguration]");
                }
                target->_name_handler.Reset(isolate, js_object);
                v8::NamedPropertyGetterCallback configuration_getter = NamedPropertyGetterCallback;
                v8::NamedPropertySetterCallback configuration_setter = nullptr;
                v8::NamedPropertyQueryCallback configuration_query = nullptr;
                v8::NamedPropertyDeleterCallback configuration_deleter = nullptr;
                v8::NamedPropertyEnumeratorCallback configuration_enumerator = nullptr;
                v8::NamedPropertyDefinerCallback configuration_definer = nullptr;
                v8::NamedPropertyDescriptorCallback configuration_descriptor = nullptr;
                if (!named_handler->get_getter(isolate).IsEmpty()) {
                    JS_THROW_ERROR(TypeError, isolate, 'Missing required option: namedHandler.getter');
                }
                if (!named_handler->get_setter(isolate).IsEmpty()) {
                    configuration_setter = NamedPropertySetterCallback;
                }
                if (!named_handler->get_query(isolate).IsEmpty()) {
                    configuration_query = NamedPropertyQueryCallback;
                }
                if (!named_handler->get_deleter(isolate).IsEmpty()) {
                    configuration_deleter = NamedPropertyDeleterCallback;
                }
                if (!named_handler->get_enumerator(isolate).IsEmpty()) {
                    configuration_enumerator = NamedPropertyEnumeratorCallback;
                }
                if (!named_handler->get_definer(isolate).IsEmpty()) {
                    configuration_definer = NamedPropertyDefinerCallback;
                }
                if (!named_handler->get_descriptor(isolate).IsEmpty()) {
                    configuration_descriptor = NamedPropertyDescriptorCallback;
                }
                v8::NamedPropertyHandlerConfiguration configuration(
                    configuration_getter,
                    configuration_setter,
                    configuration_query,
                    configuration_deleter,
                    configuration_enumerator,
                    configuration_definer,
                    configuration_descriptor,
                    interface,
                    named_handler->get_flags()
                );
                js_target->SetHandler(configuration);
            }
        }

        {
            auto name = StringTable::Get(isolate, "indexedHandler");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"indexedHandler\" is not an [object IndexedPropertyHandlerConfiguration]");
                }
                auto js_object = js_value.As<v8::Object>();
                auto indexed_handler = Object<IndexedPropertyHandlerConfiguration>::get_implementation(isolate, js_object);
                if (indexed_handler == nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"indexedHandler\" is not an [object IndexedPropertyHandlerConfiguration]");
                }
                target->_index_handler.Reset(isolate, js_object);
                v8::IndexedPropertyGetterCallbackV2 configuration_getter = nullptr;
                v8::IndexedPropertySetterCallbackV2 configuration_setter = nullptr;
                v8::IndexedPropertyQueryCallbackV2 configuration_query = nullptr;
                v8::IndexedPropertyDeleterCallbackV2 configuration_deleter = nullptr;
                v8::IndexedPropertyEnumeratorCallback configuration_enumerator = nullptr;
                v8::IndexedPropertyDefinerCallbackV2 configuration_definer = nullptr;
                v8::IndexedPropertyDescriptorCallbackV2 configuration_descriptor = nullptr;
                if (!indexed_handler->get_getter(isolate).IsEmpty()) {
                    configuration_getter = IndexedPropertyGetterCallback;
                }
                if (!indexed_handler->get_setter(isolate).IsEmpty()) {
                    configuration_setter = IndexedPropertySetterCallback;
                }
                if (!indexed_handler->get_query(isolate).IsEmpty()) {
                    configuration_query = IndexedPropertyQueryCallback;
                }
                if (!indexed_handler->get_deleter(isolate).IsEmpty()) {
                    configuration_deleter = IndexedPropertyDeleterCallback;
                }
                if (!indexed_handler->get_enumerator(isolate).IsEmpty()) {
                    configuration_enumerator = IndexedPropertyEnumeratorCallback;
                }
                if (!indexed_handler->get_definer(isolate).IsEmpty()) {
                    configuration_definer = IndexedPropertyDefinerCallback;
                }
                if (!indexed_handler->get_descriptor(isolate).IsEmpty()) {
                    configuration_descriptor = IndexedPropertyDescriptorCallback;
                }
                v8::IndexedPropertyHandlerConfiguration configuration(
                    configuration_getter,
                    configuration_setter,
                    configuration_query,
                    configuration_deleter,
                    configuration_enumerator,
                    configuration_definer,
                    configuration_descriptor,
                    interface,
                    indexed_handler->get_flags()
                );
                js_target->SetHandler(configuration);
            }
        }

        v8::Local<v8::Object> properties;
        {
            auto name = StringTable::Get(isolate, "properties");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"properties\" is not an object");
                }
                auto target_map = v8::Map::New(isolate);
                JS_EXPRESSION_IGNORE_WITH_ERROR_PREFIX(Template::Setup<ObjectTemplate>(context, interface, js_target, target_map, value.As<v8::Object>()), context, "Option \"properties\"");
                JS_EXPRESSION_RETURN(frozen_map, FrozenMap::Create(context, target_map));
                target->_properties.Reset(isolate, frozen_map);
            }
        }

        return v8::Just(target.release());
    }

    v8::Maybe<void> ObjectTemplate::SetupProperty(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::ObjectTemplate> target, v8::Local<v8::Map> map, v8::Local<v8::Value> key, v8::Local<v8::Value> value) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        if V8_LIKELY(value->IsObject()) {
            auto value_object = value.As<v8::Object>();
            auto value_accessor_property = Object<AccessorProperty>::get_implementation(isolate, value_object);
            if (value_accessor_property != nullptr) {
                if V8_UNLIKELY(key->IsObject() || key->IsExternal()) {
                    JS_THROW_ERROR(TypeError, isolate, "Template native data property key must be a primitive");
                } else if V8_UNLIKELY(!key->IsName()) {
                    JS_EXPRESSION_RETURN(key_string, key->ToString(context));
                    key = key_string;
                }
                auto key_name = key.As<v8::Name>();
                auto setter = value_accessor_property->get_setter(isolate);
                JS_EXPRESSION_IGNORE(map->Set(context, key, value));
                v8::Local<v8::Object> data;
                {
                    v8::Local<v8::Name> names[] = {
                        StringTable::Get(isolate, "descriptor"),
                        StringTable::Get(isolate, "template")
                    };
                    v8::Local<v8::Value> values[] = {
                        value,
                        interface
                    };
                    data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
                }
                target->SetAccessor(
                    key_name,
                    AccessorProperty::getter_callback,
                    !setter.IsEmpty() ? AccessorProperty::setter_callback : nullptr,
                    data,
                    value_accessor_property->get_attributes(),
                    value_accessor_property->get_getter_side_effect(),
                    value_accessor_property->get_setter_side_effect()
                );
                return v8::JustVoid();
            }
        }

        return Template::SetupProperty(context, interface, target, map, key, value);
    }

    void ObjectTemplate::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            JS_THROW_ERROR(TypeError, isolate, "Class constructor ", "ObjectTemplate", " cannot be invoked without 'new'");
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not an object.");
        }
        auto options = info[0].As<v8::Object>();

        JS_EXPRESSION_IGNORE(Create(context, info.This(), options));
        info.GetReturnValue().Set(info.This());
    }

    v8::Intercepted ObjectTemplate::NamedPropertyGetterCallback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info) {
        static const constexpr auto __function_return_type__ = [](){ return v8::Intercepted::kNo; };
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if V8_UNLIKELY (!info.Data()->IsObject()) {
            JS_THROW_ERROR(Error, isolate, "Invalid invocation: ObjectTemplate::NamedPropertyGetterCallback");
        }
        auto interface = info.Data().As<v8::Object>();
        auto js_template = Object<ObjectTemplate>::get_implementation(isolate, interface);
        if V8_UNLIKELY (js_template == nullptr) {
            JS_THROW_ERROR(Error, isolate, "Invalid invocation: ObjectTemplate::NamedPropertyGetterCallback");
        }
        auto js_descriptor = js_template->get_name_handler(isolate);
        if V8_UNLIKELY (!js_descriptor->IsObject()) {
            if (js_descriptor->IsNullOrUndefined()) {
                return v8::Intercepted::kNo;
            }
            JS_THROW_ERROR(Error, isolate, "Invalid invocation: ObjectTemplate::NamedPropertyGetterCallback");
        }
        auto named_handler = Object<ObjectTemplate::NamedPropertyHandlerConfiguration>::get_implementation(isolate, js_descriptor);
        if V8_UNLIKELY (named_handler == nullptr) {
            JS_THROW_ERROR(Error, isolate, "Invalid invocation: ObjectTemplate::NamedPropertyGetterCallback");
        }
        auto callback = named_handler->get_getter(isolate);
        if (!JS_IS_CALLABLE(callback)) {
            return v8::Intercepted::kNo;
        }

        v8::Local<v8::Object> intercept_data;
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "intercepted"),
                StringTable::Get(isolate, "value"),
            };
            v8::Local<v8::Value> values[] = {
                v8::False(isolate),
                v8::Undefined(isolate)
            };
            intercept_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }
        JS_EXPRESSION_RETURN(intercept_function, v8::Function::New(context, InterceptGetter, intercept_data, 1, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasSideEffect));
        auto data_strict = v8::Boolean::New(isolate, info.ShouldThrowOnError());

        v8::Local<v8::Object> call_data;
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "name"),
                StringTable::Get(isolate, "descriptor"),
                StringTable::Get(isolate, "template"),
                StringTable::Get(isolate, "strict")
            };
            v8::Local<v8::Value> values[] = {
                info.This(),
                info.Holder(),
                property,
                js_descriptor,
                interface,
                data_strict
            };
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }
        v8::Local<v8::Value> call_args[] = { call_data, intercept_function };
        JS_EXPRESSION_IGNORE(object_or_function_call(context, callback, v8::Undefined(isolate), sizeof(call_args) / sizeof(v8::Local<v8::Value>), call_args));
        JS_EXPRESSION_RETURN(is_intercepted, intercept_data->Get(context, StringTable::Get(isolate, "intercepted")));
        if (is_intercepted->BooleanValue(isolate)) {
            JS_EXPRESSION_RETURN(intercept_value, intercept_data->Get(context, StringTable::Get(isolate, "value")));
            info.GetReturnValue().Set(intercept_value);
            return v8::Intercepted::kYes;
        }
        return v8::Intercepted::kNo;
    }

    void ObjectTemplate::InterceptGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto interface = info.Data().As<v8::Object>();
        JS_EXPRESSION_IGNORE(interface->Set(context, StringTable::Get(isolate, "intercepted"), v8::True(isolate)));
        JS_EXPRESSION_IGNORE(interface->Set(context, StringTable::Get(isolate, "value"), info[0]));
        info.GetReturnValue().SetUndefined();
    }
}
