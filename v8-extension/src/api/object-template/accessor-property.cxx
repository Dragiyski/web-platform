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
            if V8_LIKELY (value->IsFunction() || value->IsObject() && value.As<v8::Object>()->IsCallable()) {
                target->_getter.Reset(isolate, value);
            } else {
                JS_THROW_ERROR(TypeError, isolate, "Required option \"getter\": not a function.");
            }
        }
        {
            auto name = StringTable::Get(isolate, "setter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if V8_LIKELY (value->IsFunction() || value->IsObject() && value.As<v8::Object>()->IsCallable()) {
                    target->_getter.Reset(isolate, value);
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

        target->_access_control = v8::AccessControl::DEFAULT;
        {
            auto name = StringTable::Get(isolate, "accessControl");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"accessControl\"");
                if (value != v8::AccessControl::DEFAULT && value != v8::AccessControl::ALL_CAN_READ && value != v8::AccessControl::ALL_CAN_WRITE) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"accessControl\": not a valid access control value.");
                }
                target->_access_control = static_cast<v8::AccessControl>(value);
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

        target->set_interface(isolate, info.This());
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

    v8::AccessControl ObjectTemplate::AccessorProperty::get_access_control() const {
        return _access_control;
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
        JS_EXPRESSION_RETURN(template_interface, data->Get(context, StringTable::Get(isolate, "descriptor")));
        JS_EXPRESSION_RETURN(descriptor_interface, data->Get(context, StringTable::Get(isolate, "template")));
        if V8_UNLIKELY(!template_interface->IsObject() || !descriptor_interface->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto template_implementation = Object<ObjectTemplate>::get_implementation(isolate, template_interface);
        auto descriptor_implementation = Object<AccessorProperty>::get_implementation(isolate, descriptor_interface);
        if V8_UNLIKELY(template_implementation == nullptr || descriptor_implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto getter = descriptor_implementation->get_getter(isolate);
        if (!(getter->IsFunction() || getter->IsObject() && getter.As<v8::Object>()->IsCallable())) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        
        v8::Local<v8::Object> call_data;
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "name"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "template"),
                StringTable::Get(isolate, "descriptor"),
                StringTable::Get(isolate, "strict")
            };
            v8::Local<v8::Value> values[] = {
                info.This(),
                property,
                info.Holder(),
                template_interface,
                descriptor_interface,
                v8::Boolean::New(isolate, info.ShouldThrowOnError())
            };
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }
        v8::Local<v8::Value> call_args[] = {call_data};
        JS_EXPRESSION_RETURN(call_return, object_or_function_call(context, getter, v8::Undefined(isolate), 1, call_args));
        info.GetReturnValue().Set(call_return);
    }

    void ObjectTemplate::AccessorProperty::setter_callback(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto data = info.Data().As<v8::Object>();
        JS_EXPRESSION_RETURN(template_interface, data->Get(context, StringTable::Get(isolate, "descriptor")));
        JS_EXPRESSION_RETURN(descriptor_interface, data->Get(context, StringTable::Get(isolate, "template")));
        if V8_UNLIKELY(!template_interface->IsObject() || !descriptor_interface->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto template_implementation = Object<ObjectTemplate>::get_implementation(isolate, template_interface);
        auto descriptor_implementation = Object<AccessorProperty>::get_implementation(isolate, descriptor_interface);
        if V8_UNLIKELY(template_implementation == nullptr || descriptor_implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto setter = descriptor_implementation->get_getter(isolate);
        if (!(setter->IsFunction() || setter->IsObject() && setter.As<v8::Object>()->IsCallable())) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        
        // TODO: Obtain an object wrapping the current v8::Context and add it here
        // Note: Each v8::Context has exactly one global(), we can use v8::Private on that global to reference an object wrapping the context.
        // Thus having only one JavaScript object for each v8::Context
        v8::Local<v8::Object> call_data;
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "name"),
                StringTable::Get(isolate, "value"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "template"),
                StringTable::Get(isolate, "descriptor"),
                StringTable::Get(isolate, "strict")
            };
            v8::Local<v8::Value> values[] = {
                info.This(),
                property,
                value,
                info.Holder(),
                template_interface,
                descriptor_interface,
                v8::Boolean::New(isolate, info.ShouldThrowOnError())
            };
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }
        v8::Local<v8::Value> call_args[] = {call_data};
        JS_EXPRESSION_IGNORE(object_or_function_call(context, setter, v8::Undefined(isolate), 1, call_args));
    }
}