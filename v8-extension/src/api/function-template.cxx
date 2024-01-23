#include "function-template.hxx"

#include "template.hxx"
#include "frozen-map.hxx"
#include "object-template.hxx"

#include "../js-string-table.hxx"
#include "../error-message.hxx"
#include <map>
#include <vector>

namespace dragiyski::node_ext {
    using namespace js;
    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_template;
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_template_symbol;
    }

    void FunctionTemplate::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_template.contains(isolate));
        assert(!per_isolate_template_symbol.contains(isolate));

        auto class_name = ::js::StringTable::Get(isolate, "FunctionTemplate");
        auto class_template = v8::FunctionTemplate::New(isolate, constructor, {}, {}, 1);
        class_template->SetClassName(class_name);

        auto signature = v8::Signature::New(isolate, class_template);
        auto prototype_template = class_template->PrototypeTemplate();
        {
            auto name = StringTable::Get(isolate, "get");
            auto value = v8::FunctionTemplate::New(
                isolate,
                prototype_get,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow
            );
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }

        class_template->ReadOnlyPrototype();
        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        auto template_symbol = v8::Private::New(isolate, class_name);
        per_isolate_template_symbol.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, template_symbol)
        );

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void FunctionTemplate::uninitialize(v8::Isolate* isolate) {
        per_isolate_template.erase(isolate);
        per_isolate_template_symbol.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> FunctionTemplate::get_template(v8::Isolate* isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    v8::Local<v8::Private> FunctionTemplate::get_template_symbol(v8::Isolate* isolate) {
        assert(per_isolate_template_symbol.contains(isolate));
        return per_isolate_template_symbol[isolate].Get(isolate);
    }

    void FunctionTemplate::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            JS_THROW_ERROR(TypeError, isolate, "Class constructor ", "FunctionTemplate", " cannot be invoked without 'new'");
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

    void FunctionTemplate::callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto implementation = Object<FunctionTemplate>::get_implementation(isolate, info.Data().As<v8::Object>());
        if V8_UNLIKELY(implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }

        v8::Local<v8::Value> arguments_list[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            arguments_list[i] = info[i];
        }
        auto arguments = v8::Array::New(isolate, arguments_list, info.Length());

        v8::Local<v8::Object> call_data;
        auto api_callee = implementation->get_callee(isolate);
        // TODO: Add current context here. This can (and most probably will) be different from the current context when we call the api_callee
        // Why? Because api_callee would have been given by the context that have access to NodeJS module API that imported this module
        // In contrast, the current context during this invocation would be the context of the function created from the wrapped FunctionTemplate
        // Since FunctionTemplate is context independent, the actual function will be determined by how the code retrieved it.
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "isConstructorCall"),
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "arguments"),
                StringTable::Get(isolate, "newTarget"),
                StringTable::Get(isolate, "callee"),
                StringTable::Get(isolate, "template")
            };
            v8::Local<v8::Value> values[] = {
                v8::Boolean::New(isolate, info.IsConstructCall()),
                info.This(),
                info.Holder(),
                arguments,
                info.NewTarget(),
                api_callee,
                implementation->get_interface(isolate)
            };
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, sizeof(names) / sizeof(v8::Local<v8::Name>));
        }
        v8::Local<v8::Value> call_args[] = { call_data };
        JS_EXPRESSION_RETURN(return_value, object_or_function_call(context, api_callee, v8::Undefined(isolate), sizeof(call_args) / sizeof(v8::Local<v8::Value>), call_args));
        info.GetReturnValue().Set(return_value);
    }

    void FunctionTemplate::prototype_get(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto implementation = Object<FunctionTemplate>::get_implementation(isolate, info.Holder());

        v8::Local<v8::Context> callee_context = context;

        /* if (!info[0]->IsNullOrUndefined()) {
            if (!info[0]->IsObject()) {
                JS_THROW_ERROR(TypeError, isolate, "Expected arguments[0] to be an object.");
            }
            JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(
                context_wrapper,
                Wrapper::Unwrap<Context>(isolate, info[0].As<v8::Object>(), Context::get_class_template(isolate), "Context"),
                context,
                "arguments[0]"
            );
            callee_context = context_wrapper->get_value(isolate);
            if (callee_context.IsEmpty()) {
                JS_THROW_ERROR(ReferenceError, isolate, "[object Context] no longer references v8::Context");
            }
        } */

        auto callee_template = implementation->get_value(isolate);
        JS_EXPRESSION_RETURN(callee, callee_template->GetFunction(callee_context));
        info.GetReturnValue().Set(callee);
    }

    v8::Maybe<FunctionTemplate *> FunctionTemplate::Create(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::Object> options) {
        static const constexpr auto __function_return_type__ = v8::Nothing<FunctionTemplate *>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        auto target = std::unique_ptr<FunctionTemplate>(new FunctionTemplate());

        {
            v8::Local<v8::Value> callee;
            auto name = StringTable::Get(isolate, "function");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (value->IsFunction()) {
                callee = value;
            } else if (value->IsObject() && value.As<v8::Object>()->IsCallable()) {
                callee = value;
            } else {
                JS_THROW_ERROR(TypeError, isolate, "Required option \"function\": not a function.");
            }
            target->_callee.Reset(isolate, callee);
        }
        v8::Local<v8::FunctionTemplate> receiver_template;
        {
            auto name = StringTable::Get(isolate, "receiver");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"receiver\": not an object.");
                }
                auto js_object = js_value.As<v8::Object>();
                auto receiver_implementation = Object<FunctionTemplate>::get_implementation(isolate, js_object);
                if (receiver_implementation == nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"receiver\": object does not wrap v8::FunctionTemplate.");
                }
                target->_receiver.Reset(isolate, js_object);
                receiver_template = receiver_implementation->get_value(isolate);
            }
        }
        target->_length = 0;
        {
            auto name = StringTable::Get(isolate, "length");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"length\"");
                target->_length = value;
            }
        }
        target->_allow_construct = true;
        {
            auto name = StringTable::Get(isolate, "constructor");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                target->_allow_construct = js_value->BooleanValue(isolate);
                if (!target->_allow_construct) {
                    // This change only the "default" value, option "removePrototype" can still be set to false
                    target->_remove_prototype = true;
                }
            }
        }
        target->_side_effect_type = v8::SideEffectType::kHasSideEffect;
        {
            auto name = StringTable::Get(isolate, "sideEffect");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "Option \"sideEffectType\"");
                if (
                    value == static_cast<int32_t>(v8::SideEffectType::kHasNoSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffectToReceiver)
                ) {
                    target->_side_effect_type = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"sideEffectType\": Invalid side effect type.");
                }
            }
        }
        {
            auto name = StringTable::Get(isolate, "extends");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"extends\": not an object.");
                }
                auto js_object = js_value.As<v8::Object>();
                if (Object<FunctionTemplate>::get_implementation(isolate, js_object) == nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"extends\": object does not wrap v8::FunctionTemplate.");
                }
                if (!target->_allow_construct) {
                    JS_THROW_ERROR(TypeError, isolate, "Invalid options: \"extends\" cannot be used when \"constructor\" is false");
                }
                target->_inherit.Reset(isolate, js_object);
            }
        }
        {
            auto name = StringTable::Get(isolate, "prototypeProvider");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"prototypeProvider\": not an object.");
                }
                auto js_object = js_value.As<v8::Object>();
                if (Object<FunctionTemplate>::get_implementation(isolate, js_object) == nullptr) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"prototypeProvider\": object does not wrap v8::FunctionTemplate.");
                }
                if (!target->_inherit.IsEmpty()) {
                    JS_THROW_ERROR(TypeError, isolate, "Invalid options: \"prototypeProvider\" and \"extends\" options cannot be used together");
                }
                if (!target->_allow_construct) {
                    JS_THROW_ERROR(TypeError, isolate, "Invalid options: \"prototypeProvider\" cannot be used when \"constructor\" is false");
                }
                target->_prototype_provider.Reset(isolate, js_object);
            }
        }
        {
            auto name = StringTable::Get(isolate, "name");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN(value, js_value->ToString(context));
                target->_class_name.Reset(isolate, value);
            }
        }
        {
            auto name = StringTable::Get(isolate, "readonlyPrototype");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!target->_allow_construct) {
                    JS_THROW_ERROR(TypeError, isolate, "Invalid options: \"readonlyPrototype\" cannot be used when \"constructor\" is false");
                }
                target->_readonly_prototype = js_value->BooleanValue(isolate);
            }
        }
        {
            auto name = StringTable::Get(isolate, "removePrototype");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!target->_inherit.IsEmpty()) {
                    JS_THROW_ERROR(TypeError, isolate, "Invalid options: \"extends\" and \"removePrototype\" options cannot be used together");
                }
                if (!target->_prototype_provider.IsEmpty()) {
                    JS_THROW_ERROR(TypeError, isolate, "Invalid options: \"prototypeProvider\" and \"removePrototype\" options cannot be used together");
                }
                target->_remove_prototype = js_value->BooleanValue(isolate);
            }
        }
        {
            auto name = StringTable::Get(isolate, "acceptAnyReceiver");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                target->_accept_any_receiver = js_value->BooleanValue(isolate);
            }
        }
        v8::Local<v8::Signature> signature;
        if (!receiver_template.IsEmpty()) {
            signature = v8::Signature::New(isolate, receiver_template);
        }
        auto function_template = v8::FunctionTemplate::New(isolate, callback, interface, signature, target->_length, target->_allow_construct ? v8::ConstructorBehavior::kAllow : v8::ConstructorBehavior::kThrow, target->_side_effect_type);
        {
            auto name = StringTable::Get(isolate, "properties");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, context, "Option \"properties\": Expected an [object], got ", type_of(context, js_value));
                }
                auto target_map = v8::Map::New(isolate);
                JS_EXPRESSION_IGNORE_WITH_ERROR_PREFIX(Template::Setup<FunctionTemplate>(context, interface, function_template, target_map, js_value), context, "Option \"properties\"");
                JS_EXPRESSION_RETURN(frozen_map, FrozenMap::Create(context, target_map));
                target->_properties.Reset(isolate, frozen_map);
            }
        }
        {
            auto name = StringTable::Get(isolate, "instance");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, context, "Option \"instance\": Expected an [object], got ", type_of(context, js_value));
                }
                if (!target->_allow_construct) {
                    JS_THROW_ERROR(TypeError, context, "Invalid options: Option \"instance\" cannot be used when \"constructor\" is false");
                }
                auto js_object = js_value.As<v8::Object>();
                auto instance_template = function_template->InstanceTemplate();
                JS_EXPRESSION_RETURN(interface, ObjectTemplate::get_template(isolate)->InstanceTemplate()->NewInstance(context));
                JS_EXPRESSION_IGNORE_WITH_ERROR_PREFIX(ObjectTemplate::Create(context, interface, instance_template, js_object), context, "Option \"instance\"");
                target->_instance_template.Reset(isolate, interface);
            }
        }
        {
            auto name = StringTable::Get(isolate, "prototype");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, context, "Option \"prototype\": Expected an [object], got ", type_of(context, js_value));
                }
                if (!target->_allow_construct) {
                    JS_THROW_ERROR(TypeError, context, "Invalid options: Option \"prototype\" cannot be used when \"constructor\" is false");
                }
                if (!target->_prototype_provider.IsEmpty()) {
                    JS_THROW_ERROR(TypeError, context, "Invalid options: Option \"prototype\" cannot be used together with option \"prototypeProvider\"");
                }
                auto js_object = js_value.As<v8::Object>();
                auto prototype_template = function_template->PrototypeTemplate();
                JS_EXPRESSION_RETURN(interface, ObjectTemplate::get_template(isolate)->InstanceTemplate()->NewInstance(context));
                JS_EXPRESSION_IGNORE_WITH_ERROR_PREFIX(ObjectTemplate::Create(context, interface, prototype_template, js_object), context, "Option \"prototype\"");
                target->_prototype_template.Reset(isolate, interface);
            }
        }
        auto implementation = target.release();
        implementation->set_interface(isolate, interface);
        return v8::Just(implementation);
    }

    v8::Maybe<void> FunctionTemplate::SetupProperty(v8::Local<v8::Context> context, v8::Local<v8::Object> interface, v8::Local<v8::FunctionTemplate> target, v8::Local<v8::Map> map, v8::Local<v8::Value> key, v8::Local<v8::Value> value) {
        return Template::SetupProperty(context, interface, target, map, key, value);
    }

    v8::Local<v8::FunctionTemplate> FunctionTemplate::get_value(v8::Isolate* isolate) const {
        return _value.Get(isolate);
    }
    v8::Local<v8::Value> FunctionTemplate::get_callee(v8::Isolate* isolate) const {
        return _callee.Get(isolate);
    }
}
