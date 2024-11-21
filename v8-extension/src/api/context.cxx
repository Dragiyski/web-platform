#include "context.hxx"

#include "../js-string-table.hxx"
#include "../function.hxx"
#include <map>

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_template;
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void Context::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = ::js::StringTable::Get(isolate, "Context");
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
        class_template->SetClassName(class_name);
        auto prototype_template = class_template->PrototypeTemplate();
        auto signature = v8::Signature::New(isolate, class_template);
        {
            auto name = StringTable::Get(isolate, "current");
            auto getter = v8::FunctionTemplate::New(
                isolate,
                static_get_current,
                {},
                {},
                0,
                v8::ConstructorBehavior::kThrow
            );
            getter->SetClassName(name);
            class_template->SetAccessorProperty(name, getter, {}, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "incumbent");
            auto getter = v8::FunctionTemplate::New(
                isolate,
                static_get_incumbent,
                {},
                {},
                0,
                v8::ConstructorBehavior::kThrow
            );
            getter->SetClassName(name);
            class_template->SetAccessorProperty(name, getter, {}, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "entered");
            auto getter = v8::FunctionTemplate::New(
                isolate,
                static_get_entered,
                {},
                {},
                0,
                v8::ConstructorBehavior::kThrow
            );
            getter->SetClassName(name);
            class_template->SetAccessorProperty(name, getter, {}, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "for");
            auto value = v8::FunctionTemplate::New(
                isolate,
                static_for,
                {},
                {},
                1,
                v8::ConstructorBehavior::kThrow
            );
            value->SetClassName(name);
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "global");
            auto value = v8::FunctionTemplate::New(
                isolate,
                prototype_get_global,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow
            );
            value->SetClassName(name);
            prototype_template->SetAccessorProperty(name, value, {}, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "compileFunction");
            auto value = v8::FunctionTemplate::New(
                isolate,
                prototype_compile_function,
                {},
                signature,
                1,
                v8::ConstructorBehavior::kThrow
            );
            value->SetClassName(name);
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }

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

        Object<Context>::initialize(isolate);
    }

    void Context::uninitialize(v8::Isolate* isolate) {
        Object<Context>::uninitialize(isolate);
        per_isolate_template.erase(isolate);
        per_isolate_class_symbol.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> Context::get_template(v8::Isolate* isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    v8::Local<v8::Private> Context::get_class_symbol(v8::Isolate* isolate) {
        assert(per_isolate_class_symbol.contains(isolate));
        return per_isolate_class_symbol[isolate].Get(isolate);
    }

    void Context::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if V8_UNLIKELY(!info.IsConstructCall()) {
            JS_THROW_ERROR(TypeError, isolate, "Class constructor ", "Context", " cannot be invoked without 'new'");
        }

        if (!get_template(isolate)->HasInstance(info.This())) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal constructor");
        }

        // TODO: Allow single argument - a wrapper for object template or function template (whose InstanceTemplate would be used).
        // if (info.Length() < 1) {
        //     JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        // }
        // if (!info[0]->IsObject()) {
        //     JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        // }

        // Access to "object" for global is too complex. The idea of having v8::Local<v8::Value> as global is
        // to reuse it upon creation of multiple contexts. The state of the object (its map) will be changed,
        // thus properties will be reinitialized, but the object identity would remain.

        // Instead we shall only access globalTemplate here. This shall be the only way to pre-initialize a context.
        // Anything else must be done by retrieving the global object after context creation.

        auto new_context = v8::Context::New(
            isolate,
            nullptr,
            {},
            {},
            v8::DeserializeInternalFieldsCallback(),
            context->GetMicrotaskQueue()
        );

        JS_EXPRESSION_IGNORE(new_context->Global()->SetPrivate(context, Context::get_class_symbol(isolate), info.This()));

        auto implementation = new Context(isolate, new_context);
        implementation->set_interface(isolate, info.This());

        info.GetReturnValue().Set(info.This());
    }

    void Context::static_get_current(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        JS_EXPRESSION_RETURN(self, get_context_holder(context, context));
        info.GetReturnValue().Set(self);
        return;
    }

    void Context::static_get_incumbent(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();
        auto target_context = isolate->GetIncumbentContext();
        if (target_context.IsEmpty()) {
            return;
        }

        JS_EXPRESSION_RETURN(self, get_context_holder(context, target_context));
        info.GetReturnValue().Set(self);
        return;
    }

    void Context::static_get_entered(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();
        auto target_context = isolate->GetEnteredOrMicrotaskContext();
        if (target_context.IsEmpty()) {
            return;
        }

        JS_EXPRESSION_RETURN(self, get_context_holder(context, target_context));
        info.GetReturnValue().Set(self);
        return;
    }

    v8::MaybeLocal<v8::Object> Context::get_context_holder(v8::Local<v8::Context> context, v8::Local<v8::Context> target_context) {
        using __function_return_type__ = v8::MaybeLocal<v8::Object>;
        auto isolate = context->GetIsolate();
        auto global = target_context->Global();
        auto class_symbol = Context::get_class_symbol(isolate);
        {
            JS_EXPRESSION_RETURN(has_holder, global->HasPrivate(target_context, class_symbol));
            if (!has_holder) {
                goto create_new_holder;
            }
            JS_EXPRESSION_RETURN(existing_holder, global->GetPrivate(target_context, class_symbol));
            if V8_UNLIKELY(!existing_holder->IsObject()) {
                JS_EXPRESSION_IGNORE(global->DeletePrivate(target_context, class_symbol));
                goto create_new_holder;
            }
            auto wrapper = Object<Context>::get_implementation(isolate, existing_holder.As<v8::Object>());
            if V8_UNLIKELY(wrapper == nullptr) {
                JS_EXPRESSION_IGNORE(global->DeletePrivate(target_context, class_symbol));
                goto create_new_holder;
            }
            auto wrapper_value = wrapper->get_value(isolate);
            if V8_UNLIKELY(context.IsEmpty()) {
                JS_EXPRESSION_IGNORE(global->DeletePrivate(target_context, class_symbol));
                wrapper->clear_interface(isolate);
                goto create_new_holder;
            }
            if V8_UNLIKELY(!wrapper_value->Global()->SameValue(global)) {
                JS_EXPRESSION_IGNORE(global->DeletePrivate(target_context, class_symbol));
                wrapper->clear_interface(isolate);
                goto create_new_holder;
            }
            return existing_holder.As<v8::Object>();
        }
    create_new_holder:
        {
            auto class_template = get_template(isolate);
            // Instances must be created in the control context, otherwise access checks may fail.
            JS_EXPRESSION_RETURN(holder, class_template->InstanceTemplate()->NewInstance(context));
            JS_EXPRESSION_IGNORE(global->SetPrivate(target_context, class_symbol, holder));
            auto wrapper = new Context(isolate, target_context);
            wrapper->set_interface(isolate, holder);
            return holder;
        }
    }

    void Context::static_for(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        }

        auto object = info[0].As<v8::Object>();
        info.GetReturnValue().SetUndefined();
        // Might return empty handle with no exception, in this case, undefined is returned.
        JS_EXPRESSION_RETURN(creation_context, object->GetCreationContext());
        if (creation_context.IsEmpty()) {
            return;
        }

        JS_EXPRESSION_RETURN(holder, get_context_holder(context, creation_context));
        info.GetReturnValue().Set(holder);
        return;
    }

    void Context::prototype_get_global(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto implementation = get_implementation(isolate, info.This());
        if V8_UNLIKELY(implementation == nullptr) {
            v8::Local<v8::String> receiver_type;
            JS_EXPRESSION_RETURN(receiver, type_of(context, info.This()));
            JS_THROW_ERROR(TypeError, isolate, "Context", ".", "prototype", ".", "global", " called on incompatible receiver ", receiver);
        }
        auto target_context = implementation->get_value(isolate);
        if V8_UNLIKELY(target_context.IsEmpty()) {
            return;
        }
        info.GetReturnValue().Set(target_context->Global());
    }

    void Context::prototype_compile_function(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        }

        auto options = info[0].As<v8::Object>();

        v8::Local<v8::String> function_name;
        {
            auto name = StringTable::Get(isolate, "name");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsString()) {
                    JS_THROW_ERROR(TypeError, isolate, "option `name`: not a string");
                }
                function_name = js_value.As<v8::String>();
            }
        }

        v8::Local<v8::Array> arguments;
        {
            auto name = StringTable::Get(isolate, "arguments");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsArray()) {
                    JS_THROW_ERROR(TypeError, isolate, "option `arguments`: not an array");
                }
                arguments = js_value.As<v8::Array>();
            }
        }

        v8::Local<v8::Array> scopes;
        {
            auto name = StringTable::Get(isolate, "scopes");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsArray()) {
                    JS_THROW_ERROR(TypeError, isolate, "option `scopes`: not an array");
                }
                arguments = js_value.As<v8::Array>();
            }
        }

        auto arguments_length = arguments.IsEmpty() ? 0 : arguments->Length();
        std::vector<v8::Local<v8::String>> arguments_list;
        if (arguments_length > 0) {
            try {
                arguments_list.reserve(arguments_length);
            } catch (std::bad_alloc&) {
                JS_THROW_ERROR(Error, isolate, "option `arguments`: out of memory");
            } catch (std::length_error&) {
                JS_THROW_ERROR(RangeError, isolate, "option `arguments`: too many values");
            }
            for (decltype(arguments_length) i = 0; i < arguments_length; ++i) {
                JS_EXPRESSION_RETURN(value, arguments->Get(context, i));
                if (!value->IsString()) {
                    JS_THROW_ERROR(TypeError, isolate, "option `arguments[", i, "]`: not a string")
                }
                arguments_list[i] = value.As<v8::String>();
            }
        }
        auto arguments_data = arguments_length > 0 ? arguments_list.data() : nullptr;

        auto scopes_length = scopes.IsEmpty() ? 0 : scopes->Length();
        std::vector<v8::Local<v8::Object>> scopes_list;
        if (scopes_length > 0) {
            try {
                scopes_list.reserve(scopes_length);
            } catch (std::bad_alloc&) {
                JS_THROW_ERROR(Error, isolate, "option `scopes`: out of memory");
            } catch (std::length_error&) {
                JS_THROW_ERROR(RangeError, isolate, "option `scopes`: too many values");
            }
            for (decltype(scopes_length) i = 0; i < scopes_length; ++i) {
                JS_EXPRESSION_RETURN(value, scopes->Get(context, i));
                if (!value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "option `scopes[", i, "]`: not an object")
                }
                scopes_list[i] = value.As<v8::Object>();
            }
        }
        auto scopes_data = scopes_length > 0 ? scopes_list.data() : nullptr;

        auto source = source_from_object(context, options);
        if (!source) {
            return;
        }

        auto implementation = get_implementation(isolate, info.This());
        if V8_UNLIKELY(implementation == nullptr) {
            v8::Local<v8::String> receiver_type;
            JS_EXPRESSION_RETURN(receiver, type_of(context, info.This()));
            JS_THROW_ERROR(TypeError, isolate, "Context", ".", "prototype", ".", "compileFunction", " called on incompatible receiver ", receiver);
        }
        auto target_context = implementation->get_value(isolate);
        if V8_UNLIKELY(target_context.IsEmpty()) {
            JS_THROW_ERROR(ReferenceError, isolate, "the wrapped context is already disposed");
        }

        JS_EXPRESSION_RETURN(compiled_function, v8::ScriptCompiler::CompileFunction(
            target_context,
            source.get(),
            arguments_length,
            arguments_data,
            scopes_length,
            scopes_data,
            v8::ScriptCompiler::kEagerCompile
        ));

        if (!function_name.IsEmpty()) {
            compiled_function->SetName(function_name);
        }

        info.GetReturnValue().Set(compiled_function);
    }

    v8::Local<v8::Context> Context::get_value(v8::Isolate* isolate) const {
        return _value.Get(isolate);
    }

    Context::Context(v8::Isolate* isolate, v8::Local<v8::Context> value) : 
        _value(isolate, value) {}
};