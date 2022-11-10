#include "compile-function.h"
#include "js-helper.h"
#include "function.h"

void js_compile_function(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object");
    }
    auto options = info[0].As<v8::Object>();

    uint32_t arguments_length = 0;
    v8::Local<v8::Array> js_array_arguments;
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_arguments, ToString(context, "arguments"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_value_arguments, options->Get(context, string_arguments));
    if (!js_value_arguments->IsNullOrUndefined()) {
        if (!js_value_arguments->IsArray()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option `", string_arguments, "` to be an object.");
        }
        js_array_arguments = js_value_arguments.As<v8::Array>();
        arguments_length = js_array_arguments->Length();
    }
    v8::Local<v8::String> argument_names[arguments_length];
    for (decltype(arguments_length) i = 0; i < arguments_length; ++i) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_value, js_array_arguments->Get(context, i));
        if (!js_value->IsString()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option `", string_arguments, "[", i, "]` to be a string.");
        }
        argument_names[i] = js_value.As<v8::String>();
    }

    uint32_t scopes_length = 0;
    v8::Local<v8::Array> js_array_scopes;
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_scopes, ToString(context, "scopes"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_value_scopes, options->Get(context, string_scopes));
    if (!js_value_scopes->IsNullOrUndefined()) {
        if (!js_value_scopes->IsArray()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option `", string_scopes, "` to be an object.");
        }
        js_array_scopes = js_value_scopes.As<v8::Array>();
        scopes_length = js_array_scopes->Length();
    }
    v8::Local<v8::Object> scopes[scopes_length];
    for (decltype(scopes_length) i = 0; i < scopes_length; ++i) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_value, js_array_arguments->Get(context, i));
        if (!js_value->IsObject()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option `", string_scopes, "[", i, "]` to be an object.");
        }
        scopes[i] = js_value.As<v8::Object>();
    }

    v8::Local<v8::String> name;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::String, js_value, context, options, "name");
        if (!js_value->IsNullOrUndefined()) {
            if(!js_value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option `name` to be a string.");
            }
            name = js_value;
        }
    }

    v8::Local<v8::Context> creation_context = context;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Object, js_value, context, options, "context");
        if (!js_value->IsNullOrUndefined()) {
            if(!js_value->IsObject()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option `context` to be an object.");
            }
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, value, js_value->GetCreationContext());
            creation_context = value;
        }
    }

    auto source = source_from_object(context, options);
    if (!source) {
        return;
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, result, v8::ScriptCompiler::CompileFunction(creation_context, source.get(), arguments_length, arguments_length > 0 ? argument_names : nullptr, scopes_length, scopes_length > 0 ? scopes : nullptr, v8::ScriptCompiler::kEagerCompile));
    if (!name.IsEmpty()) {
        result->SetName(name);
    }
    info.GetReturnValue().Set(result);
}