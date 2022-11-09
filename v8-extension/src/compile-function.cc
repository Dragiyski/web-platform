#include "compile-function.h"
#include "js-helper.h"

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

    {
        v8::Local<v8::Value> location;
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "location");
            if (!js_value->IsNullOrUndefined()) {
                location = js_value;
            }
        }

        if (!location.IsEmpty()) {
            int line_offset = 0, column_offset = 0;
            v8::Local<v8::Value> source_map_url;
            int script_id = -1;
            bool is_shared_cross_origin = false, is_opaque = false, is_wasm = false, is_module = false;
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "lineOffset");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
                    line_offset = value;
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "columnOffset");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
                    column_offset = value;
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "scriptId");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
                    script_id = value;
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "sourceMapUrl");
                if (!js_value->IsNullOrUndefined()) {
                    source_map_url = js_value;
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "isSharedCrossOrigin");
                if (!js_value->IsNullOrUndefined()) {
                    is_shared_cross_origin = js_value->BooleanValue(isolate);
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "isOpaque");
                if (!js_value->IsNullOrUndefined()) {
                    is_opaque = js_value->BooleanValue(isolate);
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "isWASM");
                if (!js_value->IsNullOrUndefined()) {
                    is_wasm = js_value->BooleanValue(isolate);
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "isModule");
                if (!js_value->IsNullOrUndefined()) {
                    is_module = js_value->BooleanValue(isolate);
                }
            }
        }
    }
}