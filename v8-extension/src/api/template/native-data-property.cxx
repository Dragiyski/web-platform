#include "native-data-property.hxx"

#include <cassert>
#include <map>

#include "../../error-message.hxx"
#include "../../js-string-table.hxx"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_template;
    }

    void Template::NativeDataProperty::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_template.contains(isolate));

        auto class_name = StringTable::Get(isolate, "NativeDataProperty");
        auto class_template = v8::FunctionTemplate::New(isolate, constructor);
        class_template->SetClassName(class_name);

        // Makes prototype *property* (not object) immutable similar to class X {}; syntax;
        class_template->ReadOnlyPrototype();

        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void Template::NativeDataProperty::uninitialize(v8::Isolate* isolate) {
        per_isolate_template.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> Template::NativeDataProperty::get_template(v8::Isolate *isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    void Template::NativeDataProperty::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            v8::Local<v8::Value> args[] = { info[0] };
            JS_EXPRESSION_RETURN(callee, get_template(isolate)->GetFunction(context));
            JS_EXPRESSION_RETURN(return_value, callee->NewInstance(context, 1, args));
            info.GetReturnValue().Set(return_value);
            return;
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not an object.");
        }
        auto options = info[0].As<v8::Object>();
        auto implementation = std::unique_ptr<NativeDataProperty>(new NativeDataProperty());

        {
            auto name = StringTable::Get(isolate, "getter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if V8_UNLIKELY(!JS_IS_CALLABLE(value)) {
                JS_THROW_ERROR(TypeError, isolate, "Required option \"getter\": not a function.");
            }
            implementation->_getter.Reset(isolate, value);
        }

        {
            auto name = StringTable::Get(isolate, "setter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if V8_UNLIKELY(!JS_IS_CALLABLE(value)) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"setter\": not a function.");
                }
                implementation->_setter.Reset(isolate, value);
            }
        }

        implementation->_attributes = JS_PROPERTY_ATTRIBUTE_DEFAULT;
        {
            auto name = StringTable::Get(isolate, "attributes");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"attributes\"");
                value = value & static_cast<uint32_t>(JS_PROPERTY_ATTRIBUTE_ALL);
                implementation->_attributes = static_cast<v8::PropertyAttribute>(value);
            }
        }

        implementation->_access_control = v8::AccessControl::DEFAULT;
        {
            auto name = StringTable::Get(isolate, "accessControl");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"accessControl\"");
                if V8_UNLIKELY(value != v8::AccessControl::DEFAULT && value != v8::AccessControl::ALL_CAN_READ && value != v8::AccessControl::ALL_CAN_WRITE) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"accessControl\": not a valid access control value.");
                }
                implementation->_access_control = static_cast<v8::AccessControl>(value);
            }
        }

        implementation->_getter_side_effect = v8::SideEffectType::kHasSideEffect;
        {
            auto name = StringTable::Get(isolate, "getterSideEffect");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "Option \"getterSideEffect\"");
                if V8_LIKELY(
                    value == static_cast<int32_t>(v8::SideEffectType::kHasNoSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffectToReceiver)
                ) {
                    implementation->_getter_side_effect = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"getterSideEffect\": Invalid side effect type.");
                }
            }
        }

        implementation->_setter_side_effect = v8::SideEffectType::kHasSideEffect;
        {
            auto name = StringTable::Get(isolate, "setterSideEffect");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "Option \"setterSideEffect\"");
                if V8_LIKELY(
                    value == static_cast<int32_t>(v8::SideEffectType::kHasNoSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffectToReceiver)
                ) {
                    implementation->_setter_side_effect = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"setterSideEffect\": Invalid side effect type.");
                }
            }
        }

        implementation.release()->set_interface(isolate, info.This());
        info.GetReturnValue().Set(info.This());
    }

    v8::Local<v8::Value> Template::NativeDataProperty::get_getter(v8::Isolate *isolate) const {
        return _getter.Get(isolate);
    }

    v8::Local<v8::Value> Template::NativeDataProperty::get_setter(v8::Isolate *isolate) const {
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

    void Template::NativeDataProperty::getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto data = info.Data().As<v8::Object>();
        JS_EXPRESSION_RETURN(js_descriptor, data->Get(context, StringTable::Get(isolate, "descriptor")));
        if V8_UNLIKELY(!js_descriptor->IsObject()) {
            info.GetReturnValue().SetUndefined();
            return;
        }
        auto descriptor = Object<NativeDataProperty>::get_implementation(isolate, js_descriptor.As<v8::Object>());
        if V8_UNLIKELY(descriptor == nullptr) {
            info.GetReturnValue().SetUndefined();
            return;
        }

        JS_EXPRESSION_RETURN(js_template, data->Get(context, StringTable::Get(isolate, "template")));
        if V8_UNLIKELY(!js_template->IsObject()) {
            info.GetReturnValue().SetUndefined();
            return;
        }
        auto data_strict = v8::Boolean::New(isolate, info.ShouldThrowOnError());

        v8::Local<v8::Object> call_data;
        {
            // TODO: Obtain wrapper of v8::Context of the GetCurrentContext()
            // Note: isolate->GetCurrentContext() might be different from the context of the getter call.
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
                js_template,
                data_strict
            };
            static_assert(sizeof(names) / sizeof(v8::Local<v8::Name>) == sizeof(values) / sizeof(v8::Local<v8::Value>));
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }
        auto getter = descriptor->get_getter(isolate);
        v8::Local<v8::Value> call_args[] = {call_data};
        JS_EXPRESSION_RETURN(call_return, object_or_function_call(context, getter, v8::Undefined(isolate), sizeof(call_args) / sizeof(v8::Local<v8::Value>), call_args));
        info.GetReturnValue().Set(call_return);
    }

    void Template::NativeDataProperty::setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto data = info.Data().As<v8::Object>();
        JS_EXPRESSION_RETURN(js_descriptor, data->Get(context, StringTable::Get(isolate, "descriptor")));
        if V8_UNLIKELY(!js_descriptor->IsObject()) {
            return;
        }
        auto descriptor = Object<NativeDataProperty>::get_implementation(isolate, js_descriptor.As<v8::Object>());
        if V8_UNLIKELY(descriptor == nullptr) {
            return;
        }

        JS_EXPRESSION_RETURN(js_template, data->Get(context, StringTable::Get(isolate, "template")));
        if V8_UNLIKELY(!js_template->IsObject()) {
            return;
        }
        auto data_strict = v8::Boolean::New(isolate, info.ShouldThrowOnError());

        v8::Local<v8::Object> call_data;
        {
            // TODO: Obtain wrapper of v8::Context of the GetCurrentContext()
            // Note: isolate->GetCurrentContext() might be different from the context of the getter call.
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "name"),
                StringTable::Get(isolate, "value"),
                StringTable::Get(isolate, "descriptor"),
                StringTable::Get(isolate, "template"),
                StringTable::Get(isolate, "strict")
            };
            v8::Local<v8::Value> values[] = {
                info.This(),
                info.Holder(),
                property,
                value,
                js_descriptor,
                js_template,
                data_strict
            };
            static_assert(sizeof(names) / sizeof(v8::Local<v8::Name>) == sizeof(values) / sizeof(v8::Local<v8::Value>));
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }
        auto setter = descriptor->get_setter(isolate);
        v8::Local<v8::Value> call_args[] = {call_data};
        JS_EXPRESSION_RETURN(call_return, object_or_function_call(context, setter, v8::Undefined(isolate), sizeof(call_args) / sizeof(v8::Local<v8::Value>), call_args));
        info.GetReturnValue().Set(call_return);
    }
}