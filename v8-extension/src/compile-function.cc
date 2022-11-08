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
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option ", string_arguments, " to be an object.");
        }
        js_array_arguments = js_value_arguments.As<v8::Array>();
        arguments_length = js_array_arguments->Length();
    }
    v8::Local<v8::String> argument_names[arguments_length];
    for (decltype(arguments_length) i = 0; i < arguments_length; ++i) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_value, js_array_arguments->Get(context, i));
        if (!js_value->IsString()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option ", string_arguments, " to be an object.");
        }
    }
}