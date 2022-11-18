#include "context.h"

#include <cassert>
#include <map>
#include <set>
#include <list>
#include "../js-helper.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <ranges>
#include <variant>
#include <vector>

#include "../string-table.h"
#include "../function.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    DECLARE_API_WRAPPER_BODY(Context);

    Maybe<void> Context::initialize_template(v8::Isolate* isolate, Local<v8::FunctionTemplate> class_template) {
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "for");
            auto value = v8::FunctionTemplate::New(
                isolate,
                for_object,
                {},
                {},
                1,
                v8::ConstructorBehavior::kThrow
            );
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        return v8::JustVoid();
    }

    void Context::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal constructor");
    }

    MaybeLocal<v8::Context> Context::GetCreationContext(Local<v8::Context> context, Local<v8::Object> object) {
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        Local<v8::Context> creation_context;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            auto maybe_context = object->GetCreationContext();
            if (!maybe_context.IsEmpty()) {
                creation_context = maybe_context.ToLocalChecked();
                goto has_context;
            }
            if (!try_catch.HasCaught()) {
                goto empty_context;
            }
            try_catch.ReThrow();
            return JS_NOTHING(v8::Context);
        }
    empty_context:
        JS_THROW_ERROR(JS_NOTHING(v8::Context), isolate, ReferenceError, "No creation context found");
    has_context:
        assert(!creation_context.IsEmpty());
        return creation_context;
    }

    MaybeLocal<v8::Object> Context::ForObject(Local<v8::Context> context, Local<v8::Object> object) {
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Context, creation_context, GetCreationContext(context, object));
        auto global = creation_context->Global();
        auto key = symbol_this(isolate);
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Value, holder_value, global->GetPrivate(context, key));
        if (holder_value->IsObject()) {
            return holder_value.As<v8::Object>();
        }

        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Object, holder, get_template(isolate)->InstanceTemplate()->NewInstance(context));
        JS_EXECUTE_IGNORE(JS_NOTHING(v8::Object), global->SetPrivate(context, key, holder));
        auto wrapper = new Context(isolate, creation_context);
        wrapper->Wrap(holder, holder);

        return holder;
    }

    void Context::for_object(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
        }

        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, holder, ForObject(context, info[0].As<v8::Object>()));

        JS_EXECUTE_RETURN(NOTHING, Context*, wrapper, Context::unwrap(isolate, holder));
        auto self = wrapper->container(isolate);
        info.GetReturnValue().Set(self);
    }

    void Context::compile_function(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
        }

        Local<v8::Object> options = info[0].As<v8::Object>();
        Local<v8::Context> callee_context = context;
        {
            JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "context");
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "option `context`: not an object");
                }
                JS_EXECUTE_RETURN(NOTHING, Context *, wrapper, Context::unwrap(isolate, js_value.As<v8::Object>()));
                callee_context = wrapper->value(isolate);
            }
        }

        Local<v8::Array> arguments;
        {
            JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "arguments");
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsArray()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "option `arguments`: not an array");
                }
                arguments = js_value.As<v8::Array>();
            }
        }

        Local<v8::Array> scopes;
        {
            JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "scopes");
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsArray()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "option `scopes`: not an array");
                }
                scopes = js_value.As<v8::Array>();
            }
        }

        Local<v8::String> name;
        {
            JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "name");
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsString()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "option `name` not a string");
                }
                name = js_value.As<v8::String>();
            }
        }

        auto arguments_length = arguments.IsEmpty() ? 0 : arguments->Length();
        std::vector<Local<v8::String>> arguments_list;
        if (arguments_length > 0) {
            try {
                arguments_list.reserve(arguments_length);
            } catch (std::bad_alloc &) {
                JS_THROW_ERROR(NOTHING, isolate, Error, "option `arguments`: out of memory");
            } catch (std::length_error &) {
                JS_THROW_ERROR(NOTHING, isolate, RangeError, "option `arguments`: too many values");
            }
            for (decltype(arguments_length) i = 0; i < arguments_length; ++i) {
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, value, arguments->Get(context, i));
                if (!value->IsString()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "option `arguments[", i, "]`: not a string")
                }
                arguments_list[i] = value.As<v8::String>();
            }
        }
        auto scopes_length = scopes.IsEmpty() ? 0 : scopes->Length();
        std::vector<Local<v8::Object>> scopes_list;
        if (scopes_length > 0) {
            try {
                scopes_list.reserve(scopes_length);
            } catch (std::bad_alloc &) {
                JS_THROW_ERROR(NOTHING, isolate, Error, "option `scopes`: out of memory");
            } catch (std::length_error &) {
                JS_THROW_ERROR(NOTHING, isolate, RangeError, "option `scopes`: too many values");
            }
            for (decltype(scopes_length) i = 0; i < scopes_length; ++i) {
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, value, scopes->Get(context, i));
                if (!value->IsObject()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "option `scopes[", i, "]`: not an object")
                }
                scopes_list[i] = value.As<v8::Object>();
            }
        }

        auto source = source_from_object(context, options);

        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, callee, v8::ScriptCompiler::CompileFunction(
            callee_context,
            source.get(),
            arguments_length,
            arguments_length > 0 ? arguments_list.data() : nullptr,
            scopes_length,
            scopes_length > 0 ? scopes_list.data() : nullptr,
            v8::ScriptCompiler::CompileOptions::kEagerCompile
        ));

        if (!name.IsEmpty()) {
            callee->SetName(name);
        }

        info.GetReturnValue().Set(callee);
    }
}
