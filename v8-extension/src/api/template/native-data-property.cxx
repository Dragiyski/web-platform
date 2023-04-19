#include "native-data-property.hxx"

#include <cassert>
#include <map>

#include "../../error-message.hxx"
#include "../../js-string-table.hxx"
#include "../context.hxx"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_class_template;
        std::map<v8::Isolate *, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void Template::NativeDataProperty::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_class_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = StringTable::Get(isolate, "NativeDataProperty");
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
        class_template->SetClassName(class_name);
        auto prototype_template = class_template->PrototypeTemplate();
        auto signature = v8::Signature::New(isolate, class_template);

        // Makes prototype *property* (not object) immutable similar to class X {}; syntax;
        class_template->ReadOnlyPrototype();

        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_class_symbol.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_cache)
        );

        per_isolate_class_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void Template::NativeDataProperty::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }
        info.GetReturnValue().Set(info.This());

        auto class_template = NativeDataProperty::get_class_template(isolate);

        auto holder = info.This()->FindInstanceInPrototypeChain(class_template);
        if (holder.IsEmpty() || !holder->IsObject() || holder->InternalFieldCount() < 1) {
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

        v8::Local<v8::Function> setter;
        {
            auto name = StringTable::Get(isolate, "setter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"setter\": not a function.");
                }
                setter = value.As<v8::Function>();
            }
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

        auto access_control = v8::AccessControl::DEFAULT;
        {
            auto name = StringTable::Get(isolate, "accessControl");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"accessControl\"");
                if (value != v8::AccessControl::DEFAULT && value != v8::AccessControl::ALL_CAN_READ && value != v8::AccessControl::ALL_CAN_WRITE) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"accessControl\": not a valid access control value.");
                }
                access_control = static_cast<v8::AccessControl>(value);
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

        JS_EXPRESSION_IGNORE(holder->SetPrivate(context, Wrapper::get_this_symbol(isolate), info.This()));

        auto wrapper = new NativeDataProperty(isolate, getter, setter, attributes, access_control, getter_side_effect, setter_side_effect);
        wrapper->Wrap(isolate, holder);
    }

    v8::Local<v8::Function> Template::NativeDataProperty::get_getter(v8::Isolate *isolate) const {
        return _getter.Get(isolate);
    }

    v8::Local<v8::Function> Template::NativeDataProperty::get_setter(v8::Isolate *isolate) const {
        return _setter.Get(isolate);
    }

    v8::PropertyAttribute Template::NativeDataProperty::get_attributes() const {
        return _attributes;
    }

    v8::AccessControl Template::NativeDataProperty::get_access_control() const {
        return _access_control;
    }

    v8::SideEffectType Template::NativeDataProperty::get_getter_side_effect() const {
        return _getter_side_effect;
    }

    v8::SideEffectType Template::NativeDataProperty::get_setter_side_effect() const {
        return _setter_side_effect;
    }

    v8::Maybe<void> Template::NativeDataProperty::setup(v8::Isolate *isolate, v8::Local<v8::Template> target, v8::Local<v8::Name> name, v8::Local<v8::Object> js_template_wrapper) const {
        static constexpr const auto __function_return_type__ = v8::Nothing<void>;
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        JS_EXPRESSION_RETURN(control_context, js_template_wrapper->GetCreationContext());
        if V8_UNLIKELY (control_context.IsEmpty()) {
            control_context = context;
        }

        auto this_symbol = Wrapper::get_this_symbol(isolate);

        JS_EXPRESSION_RETURN(js_context_holder, Context::get_context_holder(control_context, context));
        JS_EXPRESSION_RETURN(js_context_wrapper, js_context_holder->GetPrivate(control_context, this_symbol));

        auto this_holder = get_holder(isolate);
        JS_EXPRESSION_RETURN(this_self, this_holder->GetPrivate(context, this_symbol));

        v8::Local<v8::Object> callback_data;
        {
            v8::Local<v8::Value> this_getter, this_setter;
            auto maybe_getter = get_getter(isolate);
            auto maybe_setter = get_setter(isolate);
            if V8_UNLIKELY (maybe_getter.IsEmpty()) {
                this_getter = v8::Null(isolate);
            } else {
                this_getter = maybe_getter;
            }
            if (maybe_setter.IsEmpty()) {
                this_setter = v8::Null(isolate);
            } else {
                this_setter = maybe_setter;
            }
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "context"),
                StringTable::Get(isolate, "template"),
                StringTable::Get(isolate, "name"),
                StringTable::Get(isolate, "getter"),
                StringTable::Get(isolate, "setter"),
                StringTable::Get(isolate, "descriptor"),
            };
            v8::Local<v8::Value> values[] = {
                js_context_wrapper,
                js_template_wrapper,
                name,
                this_getter,
                this_setter,
                this_self,
            };
            callback_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
            JS_EXPRESSION_IGNORE(callback_data->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen));
        }
        target->SetNativeDataProperty(
            name,
            getter_callback,
            setter_callback,
            callback_data,
            get_attributes(),
            get_access_control(),
            get_getter_side_effect(),
            get_setter_side_effect()
        );
    }

    Template::NativeDataProperty::NativeDataProperty(
        v8::Isolate *isolate,
        v8::Local<v8::Function> getter,
        v8::Local<v8::Function> setter,
        v8::PropertyAttribute attributes,
        v8::AccessControl access_control,
        v8::SideEffectType getter_side_effect,
        v8::SideEffectType setter_side_effect
    ) :
        _getter(isolate, getter),
        _setter(isolate, setter),
        _attributes(attributes),
        _access_control(access_control),
        _getter_side_effect(getter_side_effect),
        _setter_side_effect(setter_side_effect) {}

    void Template::NativeDataProperty::getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info) {
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

    void Template::NativeDataProperty::setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto data = info.Data().As<v8::Object>();
        JS_EXPRESSION_RETURN(data_setter, data->Get(context, StringTable::Get(isolate, "setter")));
        if (!data_setter->IsFunction()) {
            return;
        }
        JS_EXPRESSION_RETURN(data_context, data->Get(context, StringTable::Get(isolate, "context")));
        JS_EXPRESSION_RETURN(data_template, data->Get(context, StringTable::Get(isolate, "template")));
        JS_EXPRESSION_RETURN(data_descriptor, data->Get(context, StringTable::Get(isolate, "descriptor")));
        auto data_strict = v8::Boolean::New(isolate, info.ShouldThrowOnError());

        auto callee = data_setter.As<v8::Function>();

        v8::Local<v8::Object> call_data;
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "name"),
                StringTable::Get(isolate, "value"),
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
                value,
                data_setter,
                data_descriptor,
                data_template,
                data_strict,
                data_context,
            };
            static_assert(sizeof(names) / sizeof(v8::Local<v8::Name>) == sizeof(values) / sizeof(v8::Local<v8::Value>));
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }

        v8::Local<v8::Value> call_args[] = {call_data};
        JS_EXPRESSION_IGNORE(callee->Call(context, v8::Undefined(isolate), sizeof(call_args) / sizeof(v8::Local<v8::Value>), call_args));
    }
}