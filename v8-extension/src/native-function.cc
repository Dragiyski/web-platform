#include "native-function.h"
#include "js-helper.h"

void v8_function_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    v8::Local<v8::Array> call_arguments;
    {
        v8::Local<v8::Value> array_arguments[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            array_arguments[i] = info[i];
        }
        call_arguments = v8::Array::New(isolate, array_arguments, info.Length());
    }
    auto call_this = info.This();
    auto call_target = info.NewTarget();
    v8::Local<v8::Value> args[] = { call_this, call_arguments, call_target };
    v8::MaybeLocal<v8::Value> maybe_result;
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, result, info.Data().As<v8::Function>()->Call(context, v8::Undefined(isolate), 3, args));
    info.GetReturnValue().Set(result);
}

void js_create_native_function(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object");
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_context, ToString(context, "context"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_name, ToString(context, "name"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_length, ToString(context, "length"));
    auto options = info[0].As<v8::Object>();

    v8::Local<v8::Function> callee;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Function, value, context, options, "function");
        if (!value->IsFunction()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'function' to be a function");
        }
        callee = value;
    }

    v8::ConstructorBehavior constructor_behavior = v8::ConstructorBehavior::kAllow;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Function, value, context, options, "pure");
        if (value->BooleanValue(isolate)) {
            constructor_behavior = v8::ConstructorBehavior::kThrow;
        }
    }

    v8::Local<v8::Context> callee_context = context;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Object, value, context, options, "context");
        if (!value->IsNullOrUndefined()) {
            if (!value->IsObject()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'context' to be an object");
            }
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, ctx, value->GetCreationContext());
        }
    }

    int length = 0;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "pure");
        if (!js_value->IsNullOrUndefined()) {
            JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
            length = value;
        }
    }

    v8::Local<v8::String> name;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "name");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'name' to be a string");
            }
            name = js_value.As<v8::String>();
        }
    }

    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, caller, v8::Function::New(callee_context, v8_function_callback, callee, length, constructor_behavior));
    if (!name.IsEmpty()) {
        callee->SetName(name);
    }
    info.GetReturnValue().Set(caller);
}