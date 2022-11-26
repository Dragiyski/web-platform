#include "function-template.hxx"

#include "context.hxx"

#include "../js-string-table.hxx"
#include "../error-message.hxx"
#include <map>
#include <vector>

namespace dragiyski::node_ext {
    using namespace js;
    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_class_template;
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void FunctionTemplate::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_class_template.contains(isolate));
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

    void FunctionTemplate::uninitialize(v8::Isolate* isolate) {
        per_isolate_class_template.erase(isolate);
        per_isolate_class_symbol.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> FunctionTemplate::get_class_template(v8::Isolate* isolate) {
        assert(per_isolate_class_template.contains(isolate));
        return per_isolate_class_template[isolate].Get(isolate);
    }

    v8::Local<v8::Private> FunctionTemplate::get_class_symbol(v8::Isolate* isolate) {
        assert(per_isolate_class_symbol.contains(isolate));
        return per_isolate_class_symbol[isolate].Get(isolate);
    }

    void FunctionTemplate::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }
        info.GetReturnValue().Set(info.This());

        auto function_template_template = FunctionTemplate::get_class_template(isolate);
        auto function_template_symbol = FunctionTemplate::get_class_symbol(isolate);

        auto holder = info.This()->FindInstanceInPrototypeChain(function_template_template);
        if (holder.IsEmpty() || !holder->IsObject() || holder->InternalFieldCount() < 1) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        }
        auto options = info[0].As<v8::Object>();

        // This value goes into the "data" argument of v8::FunctionTemplate::New()
        // The FunctionCallback is a C++ function that calls its info.Data()
        v8::Local<v8::Function> function;
        {
            auto name = StringTable::Get(isolate, "function");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsFunction()) {
                JS_THROW_ERROR(TypeError, isolate, "In required option \"function\": not a function.");
            }
            function = value.As<v8::Function>();
        }
        v8::Local<v8::Signature> signature;
        {
            auto name = StringTable::Get(isolate, "receiver");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "option \"receiver\": not an object.");
                }
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(
                    wrapper,
                    Wrapper::Unwrap<FunctionTemplate>(
                        isolate,
                        js_value.As<v8::Object>(),
                        function_template_template,
                        "FunctionTemplate"
                        ),
                    context,
                    "In option \"receiver\""
                );
                auto value = wrapper->value(isolate);
                if (value.IsEmpty()) {
                    JS_THROW_ERROR(ReferenceError, isolate, "option \"receiver\": ", "[object FunctionTemplate] no longer references v8::FunctionTemplate");
                }
                signature = v8::Signature::New(isolate, value);
            }
        }

        int length = 0;
        {
            auto name = StringTable::Get(isolate, "length");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Int32Value(context), context, "In option \"length\"");
                length = value;
            }
        }

        bool is_constructor = true;
        {
            auto name = StringTable::Get(isolate, "constructor");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                is_constructor = js_value->BooleanValue(isolate);
            }
        }
        auto constructor_behavior = is_constructor ? v8::ConstructorBehavior::kAllow : v8::ConstructorBehavior::kThrow;

        auto side_effect_type = v8::SideEffectType::kHasSideEffect;
        {
            auto name = StringTable::Get(isolate, "sideEffectType");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Int32Value(context), context, "In option \"sideEffectType\"");
                if (
                    value == static_cast<int32_t>(v8::SideEffectType::kHasNoSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffect) ||
                    value == static_cast<int32_t>(v8::SideEffectType::kHasSideEffectToReceiver)
                    ) {
                    side_effect_type = static_cast<v8::SideEffectType>(value);
                } else {
                    JS_THROW_ERROR(TypeError, isolate, "In option \"sideEffectType\": Invalid side effect type.");
                }
            }
        }

        v8::Local<v8::FunctionTemplate> inherit;
        {
            auto name = StringTable::Get(isolate, "inherit");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "option \"inherit\": not an object.");
                }
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(
                    wrapper,
                    Wrapper::Unwrap<FunctionTemplate>(
                        isolate,
                        js_value.As<v8::Object>(),
                        function_template_template,
                        "FunctionTemplate"
                        ),
                    context,
                    "In option \"inherit\""
                );
                auto value = wrapper->value(isolate);
                if (value.IsEmpty()) {
                    JS_THROW_ERROR(ReferenceError, isolate, "option \"receiver\": ", "[object FunctionTemplate] no longer references v8::FunctionTemplate");
                }
                inherit = value;
            }
        }

        v8::Local<v8::FunctionTemplate> prototype_provider;
        {
            auto name = StringTable::Get(isolate, "prototypeTemplate");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "option \"prototypeTemplate\": not an object.");
                }
                if (!is_constructor) {
                    JS_THROW_ERROR(TypeError, isolate, "option \"prototypeTemplate\": cannot be used when option \"constructor\" is false.");
                }
                if (!inherit.IsEmpty()) {
                    JS_THROW_ERROR(TypeError, isolate, "option \"prototypeTemplate\": cannot be object (wrapping FunctionTemplate) when inherit option is also specified");
                }
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(
                    wrapper,
                    Wrapper::Unwrap<FunctionTemplate>(
                        isolate,
                        js_value.As<v8::Object>(),
                        function_template_template,
                        "FunctionTemplate"
                        ),
                    context,
                    "In option \"prototypeTemplate\""
                );
                auto value = wrapper->value(isolate);
                if (value.IsEmpty()) {
                    JS_THROW_ERROR(ReferenceError, isolate, "option \"prototype\": ", "[object FunctionTemplate] no longer references v8::FunctionTemplate");
                }
                prototype_provider = value;
            }
        }

        v8::Local<v8::String> name;
        {
            auto property_name = StringTable::Get(isolate, "name");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, property_name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN(value, js_value->ToString(context));
                name = value;
            }
        }

        bool readonly_prototype = false;
        {
            auto name = StringTable::Get(isolate, "readonlyPrototype");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                auto value = js_value->BooleanValue(isolate);
                if (value) {
                    if (!is_constructor || !prototype_provider.IsEmpty()) {
                        JS_THROW_ERROR(TypeError, isolate, "option \"readonlyPrototype\" cannot be set ( = true) when there is no own prototype (constructor == false OR prototypeTemplate is provided).");
                    }
                    readonly_prototype = true;
                }
            }
        }

        bool accept_any_receiver = true;
        {
            auto name = StringTable::Get(isolate, "readonlyPrototype");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                accept_any_receiver = js_value->BooleanValue(isolate);
            }
        }

        // TODO: For properties "instance" and "prototype" that accept the same options format as the ObjectTemplate;
        // "prototype" is only allowed when constructor == true and prototype_provider.IsEmpty()
        // "instance" is always allowed.

        JS_EXPRESSION_IGNORE(holder->SetPrivate(context, Wrapper::get_symbol(isolate), info.This()))

            auto template_value = v8::FunctionTemplate::New(isolate, callback, holder, signature, length, constructor_behavior, side_effect_type);
        if (!name.IsEmpty()) {
            template_value->SetClassName(name);
        }
        if (!inherit.IsEmpty()) {
            template_value->Inherit(inherit);
        }
        if (!prototype_provider.IsEmpty()) {
            template_value->SetPrototypeProviderTemplate(template_value);
        }
        template_value->SetAcceptAnyReceiver(accept_any_receiver);
        if (readonly_prototype) {
            template_value->ReadOnlyPrototype();
        }

        auto wrapper = new FunctionTemplate(isolate, template_value, function);
        wrapper->Wrap(isolate, holder);
    }

    void FunctionTemplate::callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Data().As<v8::Object>();
        auto wrapper = Wrapper::Unwrap<FunctionTemplate>(isolate, holder);
        JS_EXPRESSION_RETURN(self, holder->GetPrivate(context, Wrapper::get_symbol(isolate)));
        auto callee = wrapper->callee(isolate);

        v8::Local<v8::Value> arguments_list[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            arguments_list[i] = info[i];
        }
        auto arguments = v8::Array::New(isolate, arguments_list, info.Length());

        v8::Local<v8::Object> call_data;
        {
            v8::Local<v8::Name> names[] = {
                StringTable::Get(isolate, "isConstructorCall"),
                StringTable::Get(isolate, "this"),
                StringTable::Get(isolate, "holder"),
                StringTable::Get(isolate, "arguments"),
                StringTable::Get(isolate, "newTarget"),
                StringTable::Get(isolate, "callee"),
            };
            v8::Local<v8::Value> values[] = {
                v8::Boolean::New(isolate, info.IsConstructCall()),
                info.This(),
                info.Holder(),
                arguments,
                info.NewTarget(),
                info.Data()
            };
            call_data = v8::Object::New(isolate, v8::Null(isolate), names, values, 6);
        }
        v8::Local<v8::Value> call_args[] = { call_data };
        JS_EXPRESSION_RETURN(return_value, callee->Call(context, self, 1, call_args));
        info.GetReturnValue().Set(return_value);
    }

    void FunctionTemplate::prototype_get(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto wrapper = Wrapper::Unwrap<FunctionTemplate>(isolate, info.Holder());
        if (wrapper == nullptr) {
            JS_THROW_ERROR(ReferenceError, isolate, "[object FunctionTemplate] no longer wraps a native object");
        }

        v8::Local<v8::Context> callee_context = context;

        if (!info[0]->IsNullOrUndefined()) {
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
        }

        auto callee_template = wrapper->value(isolate);
        JS_EXPRESSION_RETURN(callee, callee_template->GetFunction(callee_context));
        info.GetReturnValue().Set(callee);
    }

    v8::Local<v8::FunctionTemplate> FunctionTemplate::value(v8::Isolate* isolate) const {
        return _value.Get(isolate);
    }
    v8::Local<v8::Function> FunctionTemplate::callee(v8::Isolate* isolate) const {
        return _callee.Get(isolate);
    }

    FunctionTemplate::FunctionTemplate(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> value, v8::Local<v8::Function> callee) :
        _value(isolate, value),
        _callee(isolate, callee) {}
}
