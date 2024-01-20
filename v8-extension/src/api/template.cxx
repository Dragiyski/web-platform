#include "template.hxx"

#include "private.hxx"
#include "function-template.hxx"
#include "object-template.hxx"
#include "template/native-data-property.hxx"
#include "template/lazy-data-property.hxx"

#include "../js-string-table.hxx"
#include "../error-message.hxx"
#include <map>

namespace dragiyski::node_ext {
    using namespace js;

    namespace {
        // Used to hold a reference from an object (or function) created by ObjectTemplate or FunctionTemplate to the object wrapping that template.
        std::map<v8::Isolate *, Shared<v8::Private>> per_isolate_template_symbol;
    }

    void Template::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_template_symbol.contains(isolate));
        {
            auto name = StringTable::Get(isolate, "template");
            auto symbol = v8::Private::New(isolate, name);
            per_isolate_template_symbol.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(isolate),
                std::forward_as_tuple(isolate, symbol)
            );
        }
    }

    void Template::uninitialize(v8::Isolate *isolate) {
        per_isolate_template_symbol.erase(isolate);
    }

    v8::Local<v8::Private> Template::get_template_symbol(v8::Isolate *isolate) {
        assert(per_isolate_template_symbol.contains(isolate));
        return per_isolate_template_symbol[isolate].Get(isolate);
    }

    v8::Maybe<void> Template::SetupProperty(v8::Local<v8::Context> context, v8::Local<v8::Template> target, v8::Local<v8::Map> map, v8::Local<v8::Value> key, v8::Local<v8::Value> value) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        // value must be descriptor object (similar to how Object.defineProperties() accept an object for each property that contains configurable, value, etc.)
        if (!value->IsObject()) {
            JS_THROW_ERROR(TypeError, context, "Template property description must be an [object], got ", type_of(context, value));
        }
        auto value_object = value.As<v8::Object>();
        // 1. key is [string] or [symbol], value is [object NativeDataProperty]
        auto value_native_data_property = Object<NativeDataProperty>::get_implementation(isolate, value_object);
        if (value_native_data_property != nullptr) {
            if V8_UNLIKELY(key->IsObject() || key->IsExternal()) {
                JS_THROW_ERROR(TypeError, isolate, "Template native data property key must be a primitive");
            } else if V8_UNLIKELY(!key->IsName()) {
                JS_EXPRESSION_RETURN(key_string, key->ToString(context));
                key = key_string;
            }
            auto key_name = key.As<v8::Name>();
            auto setter = value_native_data_property->get_setter(isolate);
            JS_EXPRESSION_IGNORE(map->Set(context, key, value));
            target->SetNativeDataProperty(
                key_name,
                NativeDataProperty::getter_callback,
                !setter.IsEmpty() ? NativeDataProperty::setter_callback : nullptr,
                value_object,
                value_native_data_property->get_attributes(),
                value_native_data_property->get_access_control(),
                value_native_data_property->get_getter_side_effect(),
                value_native_data_property->get_setter_side_effect()
            );
            return v8::JustVoid();
        }
        // 2. key is [string] or [symbol], value is [object LazyDataProperty]
        auto value_lazy_data_property = Object<LazyDataProperty>::get_implementation(isolate, value_object);
        if (value_lazy_data_property != nullptr) {
            if V8_UNLIKELY(key->IsObject() || key->IsExternal()) {
                JS_THROW_ERROR(TypeError, isolate, "Template native data property key must be a primitive");
            } else if V8_UNLIKELY(!key->IsName()) {
                JS_EXPRESSION_RETURN(key_string, key->ToString(context));
                key = key_string;
            }
            auto key_name = key.As<v8::Name>();
            JS_EXPRESSION_IGNORE(map->Set(context, key, value));
            target->SetLazyDataProperty(
                key_name,
                LazyDataProperty::getter_callback,
                value_object,
                value_lazy_data_property->get_attributes(),
                value_lazy_data_property->get_getter_side_effect(),
                value_lazy_data_property->get_setter_side_effect()
            );
            return v8::JustVoid();
        }
        // 3. key is [string] or [symbol], value is [object Object] having:
        // [attributes]: property attributes integer
        // [accessControl]: property accessControl
        // [value]: the property value
        // or
        // [get]: [object FunctionTemplate]
        // [set]: [object FunctionTemplate]

        // If "get" or "set" is present, the object is considered accessor descriptor, otherwise it is data descriptor;
        // If none of "value", "get" and "set" is specified, it is assumed "value" to be [undefined].
        v8::PropertyAttribute property_attribute = v8::PropertyAttribute::None;
        v8::AccessControl access_control = v8::AccessControl::DEFAULT;
        v8::Local<v8::Value> property_value;
        FunctionTemplate* property_getter = nullptr;
        FunctionTemplate* property_setter = nullptr;
        v8::Local<v8::Object> descriptor_getter, descriptor_setter;
        {
            auto name = StringTable::Get(isolate, "attributes");
            JS_EXPRESSION_RETURN(js_value, value_object->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"attributes\"");
                value = value & static_cast<uint32_t>(JS_PROPERTY_ATTRIBUTE_ALL);
                property_attribute = static_cast<v8::PropertyAttribute>(value);
            }
        }
        {
            auto name = StringTable::Get(isolate, "accessControl");
            JS_EXPRESSION_RETURN(js_value, value_object->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"accessControl\"");
                if (value != v8::AccessControl::DEFAULT && value != v8::AccessControl::ALL_CAN_READ && value != v8::AccessControl::ALL_CAN_WRITE) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"accessControl\": not a valid access control value.");
                }
                access_control = static_cast<v8::AccessControl>(value);
            }
        }
        {
            JS_EXPRESSION_RETURN(descriptor_get, value_object->Get(context, StringTable::Get(isolate, "get")));
            if (descriptor_get->IsObject()) {
                property_getter = Object<FunctionTemplate>::get_implementation(isolate, descriptor_get.As<v8::Object>());
                if (property_getter == nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "<object descriptor>.get specified, but not an [object FunctionTemplate]");
                }
                descriptor_getter = descriptor_get.As<v8::Object>();
            }
        }
        {
            JS_EXPRESSION_RETURN(descriptor_set, value_object->Get(context, StringTable::Get(isolate, "set")));
            if (descriptor_set->IsObject()) {
                property_setter = Object<FunctionTemplate>::get_implementation(isolate, descriptor_set.As<v8::Object>());
                if (property_setter == nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "<object descriptor>.set specified, but not an [object FunctionTemplate]");
                }
                descriptor_setter = descriptor_set.As<v8::Object>();
            }
        }
        {
            auto string_value = StringTable::Get(isolate, "value");
            JS_EXPRESSION_RETURN(has_value, value_object->Has(context, string_value));
            if (has_value) {
                if (property_getter != nullptr || property_setter != nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "Invalid property descriptor. Cannot both specify accessors and a value or writable attribute.");
                }
                JS_EXPRESSION_RETURN(value, value_object->Get(context, string_value));
                property_value = value;
            }
        }
        if (property_value.IsEmpty()) {
            property_value = v8::Undefined(isolate);
        }
        // Storing the provided object in the map is not desirable, instead a new frozen object with no prototype store all resolved values.
        v8::Local<v8::Object> map_value;
        {
            v8::Local<v8::Name> map_value_keys[4] = {
                StringTable::Get(isolate, "attributes"),
                StringTable::Get(isolate, "accessControl"),
                {}, {}
            };
            v8::Local<v8::Value> map_value_values[4] = {
                v8::Integer::NewFromUnsigned(isolate, static_cast<uint32_t>(property_attribute)),
                v8::Integer::NewFromUnsigned(isolate, static_cast<uint32_t>(access_control)),
                {}, {}
            };
            std::size_t size = 2;
            if (property_getter != nullptr || property_setter != nullptr) {
                if (property_getter != nullptr) {
                    map_value_keys[size] = StringTable::Get(isolate, "get");
                    map_value_values[size] = descriptor_getter;
                    ++size;
                }
                if (property_setter != nullptr) {
                    map_value_keys[size] = StringTable::Get(isolate, "set");
                    map_value_values[size] = descriptor_setter;
                    ++size;
                }
            } else {
                map_value_keys[size] = StringTable::Get(isolate, "value");
                map_value_values[size] = property_value;
                ++size;
            }
            map_value = v8::Object::New(
                isolate,
                v8::Null(isolate),
                map_value_keys,
                map_value_values,
                size
            );
            JS_EXPRESSION_IGNORE(map_value->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen));
        }
        // 3.1. key is [string] or [symbol], the descriptor is accessor
        if (property_getter != nullptr || property_setter != nullptr) {
            if V8_UNLIKELY(key->IsObject() || key->IsExternal()) {
                JS_THROW_ERROR(TypeError, isolate, "Template native data property key must be a primitive");
            } else if V8_UNLIKELY(!key->IsName()) {
                JS_EXPRESSION_RETURN(key_string, key->ToString(context));
                key = key_string;
            }
            auto key_name = key.As<v8::Name>();
            JS_EXPRESSION_IGNORE(map->Set(context, key, map_value));
            target->SetAccessorProperty(
                key_name,
                property_getter != nullptr ? property_getter->get_value(isolate) : v8::Local<v8::FunctionTemplate>(),
                property_setter != nullptr ? property_setter->get_value(isolate) : v8::Local<v8::FunctionTemplate>(),
                property_attribute,
                access_control
            );
            return v8::JustVoid();
        }
        // 3.2. key is [object Private], descriptor is data descriptor with value one of:
        // - [object FunctionTemplate]
        // - [object ObjectTemplate]
        // - primitive
        if V8_UNLIKELY(key->IsObject()) {
            auto key_object = key.As<v8::Object>();
            auto key_private = Object<Private>::get_implementation(isolate, key_object);
            if V8_LIKELY(key_private != nullptr) {
                if (property_value->IsObject()) {
                    auto value_object = property_value.As<v8::Object>();
                    auto value_function_template = Object<FunctionTemplate>::get_implementation(isolate, value_object);
                    if (value_function_template != nullptr) {
                        JS_EXPRESSION_IGNORE(map->Set(context, key, map_value));
                        target->SetPrivate(key_private->get_value(isolate), value_function_template->get_value(isolate), property_attribute);
                        return v8::JustVoid();
                    }
                    auto value_object_template = Object<ObjectTemplate>::get_implementation(isolate, value_object);
                    if (value_object_template != nullptr) {
                        JS_EXPRESSION_IGNORE(map->Set(context, key, map_value));
                        target->SetPrivate(key_private->get_value(isolate), value_object_template->get_value(isolate), property_attribute);
                        return v8::JustVoid();
                    }
                    JS_THROW_ERROR(TypeError, context, "Template property must be a primitive, or [object FunctionTemplate], or [object ObjectTemplate], got ", type_of(context, value));
                } else if V8_UNLIKELY(property_value->IsExternal()) {
                    JS_THROW_ERROR(TypeError, context, "Template property must be a primitive, or [object FunctionTemplate], or [object ObjectTemplate], got ", type_of(context, value));
                } else {
                    JS_EXPRESSION_IGNORE(map->Set(context, key, map_value));
                    target->SetPrivate(key_private->get_value(isolate), property_value, property_attribute);
                    return v8::JustVoid();
                }
            }
            JS_THROW_ERROR(TypeError, context, "Template property name must be javascript property name or [object Private], got ", type_of(context, key));
        } else if V8_UNLIKELY(key->IsExternal()) {
            JS_THROW_ERROR(TypeError, context, "Template property name must be javascript property name or [object Private], got ", type_of(context, key));
        } else if V8_UNLIKELY(!key->IsString() && !key->IsSymbol()) {
            JS_EXPRESSION_RETURN(key_string, key->ToString(context));
            key = key_string;
        }

        auto key_name = key.As<v8::Name>();

        // 3.2. key is [string] or [symbol], descriptor is data descriptor with value one of:
        // - [object FunctionTemplate]
        // - [object ObjectTemplate]
        // - primitive
        if (property_value->IsObject()) {
            auto value_object = value.As<v8::Object>();
            auto value_function_template = Object<FunctionTemplate>::get_implementation(isolate, value_object);
            if (value_function_template != nullptr) {
                JS_EXPRESSION_IGNORE(map->Set(context, key, map_value));
                target->Set(key_name, value_function_template->get_value(isolate), property_attribute);
                return v8::JustVoid();
            }
            auto value_object_template = Object<ObjectTemplate>::get_implementation(isolate, value_object);
            if (value_object_template != nullptr) {
                JS_EXPRESSION_IGNORE(map->Set(context, key, map_value));
                target->Set(key_name, value_object_template->get_value(isolate), property_attribute);
                return v8::JustVoid();
            }
            JS_THROW_ERROR(TypeError, context, "Template property must be a primitive, or [object FunctionTemplate], or [object ObjectTemplate], got ", type_of(context, value));
        } else if V8_UNLIKELY(property_value->IsExternal()) {
            JS_THROW_ERROR(TypeError, context, "Template property must be a primitive, or [object FunctionTemplate], or [object ObjectTemplate], got ", type_of(context, value));
        } else {
            JS_EXPRESSION_IGNORE(map->Set(context, key, value));
            target->Set(key_name, property_value, property_attribute);
            return v8::JustVoid();
        }
    }
}
