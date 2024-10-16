#include "accessor-property.hxx"

#include "../frozen-map.hxx"

#include <cassert>
#include <map>

#include "../../error-message.hxx"
#include "../../js-string-table.hxx"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_template;
    }

    void ObjectTemplate::AccessorProperty::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_template.contains(isolate));

        auto class_name = StringTable::Get(isolate, "AccessorProperty");
        auto class_template = v8::FunctionTemplate::New(isolate, constructor);
        class_template->SetClassName(class_name);

        // Makes prototype *property* (not object) immutable similar to class X {}; syntax;
        class_template->ReadOnlyPrototype();

        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        auto signature = v8::Signature::New(isolate, class_template);
        auto prototype_template = class_template->PrototypeTemplate();

        {
            auto name = StringTable::Get(isolate, "get");
            auto getter = v8::FunctionTemplate::New(isolate, prototype_get_getter, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->SetAccessorProperty(name, getter, {});
        }
        {
            auto name = StringTable::Get(isolate, "set");
            auto getter = v8::FunctionTemplate::New(isolate, prototype_get_setter, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->SetAccessorProperty(name, getter, {});
        }
        {
            auto name = StringTable::Get(isolate, "attributes");
            auto getter = v8::FunctionTemplate::New(isolate, prototype_get_attributes, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->SetAccessorProperty(name, getter, {});
        }
        {
            auto name = StringTable::Get(isolate, "getterSideEffects");
            auto getter = v8::FunctionTemplate::New(isolate, prototype_get_getter_side_effect, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->SetAccessorProperty(name, getter, {});
        }
        {
            auto name = StringTable::Get(isolate, "setterSideEffects");
            auto getter = v8::FunctionTemplate::New(isolate, prototype_get_setter_side_effect, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->SetAccessorProperty(name, getter, {});
        }

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void ObjectTemplate::AccessorProperty::uninitialize(v8::Isolate *isolate) {
        per_isolate_template.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> ObjectTemplate::AccessorProperty::get_template(v8::Isolate *isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    void ObjectTemplate::AccessorProperty::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            v8::Local<v8::Value> args[] = {info[0]};
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
        auto target = std::unique_ptr<AccessorProperty>(new AccessorProperty());

        {
            auto name = StringTable::Get(isolate, "getter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if V8_LIKELY (JS_IS_CALLABLE(value)) {
                target->_getter.Reset(isolate, value);
            } else {
                JS_THROW_ERROR(TypeError, isolate, "Required option \"getter\": not a function.");
            }
        }
        {
            auto name = StringTable::Get(isolate, "setter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if V8_LIKELY (JS_IS_CALLABLE(value)) {
                    target->_setter.Reset(isolate, value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Required option \"setter\": not a function.");
                }
            }
        }

        target->_attributes = JS_PROPERTY_ATTRIBUTE_DEFAULT;
        {
            auto name = StringTable::Get(isolate, "attributes");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"attributes\"");
                value = value & static_cast<uint32_t>(JS_PROPERTY_ATTRIBUTE_ALL);
                target->_attributes = static_cast<v8::PropertyAttribute>(value);
            }
        }

        target->_getter_side_effect = v8::SideEffectType::kHasSideEffect;
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
                    target->_getter_side_effect = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"getterSideEffect\": Invalid side effect type.");
                }
            }
        }

        target->_setter_side_effect = v8::SideEffectType::kHasSideEffect;
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
                    target->_setter_side_effect = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"setterSideEffect\": Invalid side effect type.");
                }
            }
        }

        target.release()->set_interface(isolate, info.This());
        info.GetReturnValue().Set(info.This());
    }

    v8::Local<v8::Value> ObjectTemplate::AccessorProperty::get_getter(v8::Isolate *isolate) const {
        return _getter.Get(isolate);
    }

    v8::Local<v8::Value> ObjectTemplate::AccessorProperty::get_setter(v8::Isolate *isolate) const {
        return _setter.Get(isolate);
    }

    v8::PropertyAttribute ObjectTemplate::AccessorProperty::get_attributes() const {
        return _attributes;
    }

    v8::SideEffectType ObjectTemplate::AccessorProperty::get_getter_side_effect() const {
        return _getter_side_effect;
    }

    v8::SideEffectType ObjectTemplate::AccessorProperty::get_setter_side_effect() const {
        return _setter_side_effect;
    }

    void ObjectTemplate::AccessorProperty::getter_callback(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value> &info) {
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
        auto descriptor = Object<AccessorProperty>::get_implementation(isolate, js_descriptor.As<v8::Object>());
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

    void ObjectTemplate::AccessorProperty::setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto data = info.Data().As<v8::Object>();
        JS_EXPRESSION_RETURN(js_descriptor, data->Get(context, StringTable::Get(isolate, "descriptor")));
        if V8_UNLIKELY(!js_descriptor->IsObject()) {
            return;
        }
        auto descriptor = Object<AccessorProperty>::get_implementation(isolate, js_descriptor.As<v8::Object>());
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
        auto setter = descriptor->get_getter(isolate);
        v8::Local<v8::Value> call_args[] = {call_data};
        JS_EXPRESSION_RETURN(call_return, object_or_function_call(context, setter, v8::Undefined(isolate), sizeof(call_args) / sizeof(v8::Local<v8::Value>), call_args));
        info.GetReturnValue().Set(call_return);
    }

    void ObjectTemplate::AccessorProperty::prototype_get_getter(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto implementation = Object<AccessorProperty>::get_own_implementation(isolate, info.Holder());
        if V8_UNLIKELY(implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }

        auto value = implementation->_getter.Get(isolate);
        if V8_UNLIKELY(value.IsEmpty()) {
            info.GetReturnValue().SetUndefined();
        } else {
            info.GetReturnValue().Set(value);
        }
    }

    void ObjectTemplate::AccessorProperty::prototype_get_setter(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto implementation = Object<AccessorProperty>::get_own_implementation(isolate, info.Holder());
        if V8_UNLIKELY(implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }

        auto value = implementation->_setter.Get(isolate);
        if V8_UNLIKELY(value.IsEmpty()) {
            info.GetReturnValue().SetUndefined();
        } else {
            info.GetReturnValue().Set(value);
        }
    }

    void ObjectTemplate::AccessorProperty::prototype_get_attributes(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto implementation = Object<AccessorProperty>::get_own_implementation(isolate, info.Holder());
        if V8_UNLIKELY(implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }

        info.GetReturnValue().Set(static_cast<uint32_t>(implementation->_attributes));
    }

    void ObjectTemplate::AccessorProperty::prototype_get_getter_side_effect(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto implementation = Object<AccessorProperty>::get_own_implementation(isolate, info.Holder());
        if V8_UNLIKELY(implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }

        info.GetReturnValue().Set(static_cast<uint32_t>(implementation->_getter_side_effect));
    }

    void ObjectTemplate::AccessorProperty::prototype_get_setter_side_effect(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto implementation = Object<AccessorProperty>::get_own_implementation(isolate, info.Holder());
        if V8_UNLIKELY(implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }

        info.GetReturnValue().Set(static_cast<uint32_t>(implementation->_setter_side_effect));
    }
}