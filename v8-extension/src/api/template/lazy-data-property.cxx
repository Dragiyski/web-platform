#include "lazy-data-property.hxx"

#include <cassert>
#include <map>

#include "../../error-message.hxx"
#include "../../js-string-table.hxx"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_template;
    }

    void Template::LazyDataProperty::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_template.contains(isolate));

        auto class_name = StringTable::Get(isolate, "LazyDataProperty");
        auto class_template = v8::FunctionTemplate::New(isolate, constructor);
        class_template->SetClassName(class_name);
        auto prototype_template = class_template->PrototypeTemplate();
        auto signature = v8::Signature::New(isolate, class_template);

        // Makes prototype *property* (not object) immutable similar to class X {}; syntax;
        class_template->ReadOnlyPrototype();

        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void Template::LazyDataProperty::uninitialize(v8::Isolate* isolate) {
        per_isolate_template.erase(isolate);
    }

    void Template::LazyDataProperty::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not an object.");
        }
        auto options = info[0].As<v8::Object>();

        v8::Local<v8::Function> getter;
        {
            auto name = StringTable::Get(isolate, "getter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsFunction()) {
                JS_THROW_ERROR(TypeError, isolate, "Required option \"getter\": not a function.");
            }
            getter = value.As<v8::Function>();
        }

        auto attributes = JS_PROPERTY_ATTRIBUTE_DEFAULT;
        {
            auto name = StringTable::Get(isolate, "attributes");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"attributes\"");
                value = value & static_cast<uint32_t>(JS_PROPERTY_ATTRIBUTE_ALL);
                attributes = static_cast<v8::PropertyAttribute>(value);
            }
        }

        auto getter_side_effect = v8::SideEffectType::kHasSideEffect;
        {
            auto name = StringTable::Get(isolate, "getterSideEffect");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "Option \"getterSideEffect\"");
                if (
                    value == static_cast<int32_t>(v8::SideEffectType::kHasNoSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffectToReceiver)
                ) {
                    getter_side_effect = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"getterSideEffect\": Invalid side effect type.");
                }
            }
        }

        auto setter_side_effect = v8::SideEffectType::kHasSideEffect;
        {
            auto name = StringTable::Get(isolate, "setterSideEffect");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "Option \"setterSideEffect\"");
                if (
                    value == static_cast<int32_t>(v8::SideEffectType::kHasNoSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffectToReceiver)
                ) {
                    setter_side_effect = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"setterSideEffect\": Invalid side effect type.");
                }
            }
        }

        auto implementation = new LazyDataProperty(isolate, getter, attributes, getter_side_effect, setter_side_effect);
        implementation->set_interface(isolate, info.This());
        info.GetReturnValue().Set(info.This());
    }

    v8::Local<v8::Function> Template::LazyDataProperty::get_getter(v8::Isolate *isolate) const {
        return _getter.Get(isolate);
    }

    v8::PropertyAttribute Template::LazyDataProperty::get_attributes() const {
        return _attributes;
    }

    v8::SideEffectType Template::LazyDataProperty::get_getter_side_effect() const {
        return _getter_side_effect;
    }

    v8::SideEffectType Template::LazyDataProperty::get_setter_side_effect() const {
        return _setter_side_effect;
    }

    Template::LazyDataProperty::LazyDataProperty(
        v8::Isolate *isolate,
        v8::Local<v8::Function> getter,
        v8::PropertyAttribute attributes,
        v8::SideEffectType getter_side_effect,
        v8::SideEffectType setter_side_effect
    ) :
        _getter(isolate, getter),
        _attributes(attributes),
        _getter_side_effect(getter_side_effect),
        _setter_side_effect(setter_side_effect) {}

    void Template::LazyDataProperty::getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto data = info.Data().As<v8::Object>();
        JS_EXPRESSION_RETURN(data_getter, data->Get(context, StringTable::Get(isolate, "getter")));
        if (!data_getter->IsFunction()) {
            info.GetReturnValue().SetUndefined();
            return;
        }
        JS_EXPRESSION_RETURN(data_context, data->Get(context, StringTable::Get(isolate, "context")));
        JS_EXPRESSION_RETURN(data_template, data->Get(context, StringTable::Get(isolate, "template")));
        JS_EXPRESSION_RETURN(data_descriptor, data->Get(context, StringTable::Get(isolate, "descriptor")));
        auto data_strict = v8::Boolean::New(isolate, info.ShouldThrowOnError());

        auto callee = data_getter.As<v8::Function>();

        v8::Local<v8::Object> call_data;
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "name"),
                StringTable::Get(isolate, "callee"),
                StringTable::Get(isolate, "descriptor"),
                StringTable::Get(isolate, "template"),
                StringTable::Get(isolate, "shouldThrowOnError"),
                StringTable::Get(isolate, "context"),
            };
            v8::Local<v8::Value> values[] = {
                info.This(),
                info.Holder(),
                property,
                data_getter,
                data_descriptor,
                data_template,
                data_strict,
                data_context,
            };
            static_assert(sizeof(names) / sizeof(v8::Local<v8::Name>) == sizeof(values) / sizeof(v8::Local<v8::Value>));
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }

        v8::Local<v8::Value> call_args[] = {call_data};
        JS_EXPRESSION_RETURN(call_return, callee->Call(context, v8::Undefined(isolate), sizeof(call_args) / sizeof(v8::Local<v8::Value>), call_args));
        info.GetReturnValue().Set(call_return);
    }
}