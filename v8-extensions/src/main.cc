#include <node.h>
#include <v8.h>
#include "js-helper.h"

void js_get_security_token(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_INVALID_ARG_COUNT(NOTHING, context, info, 1);
    }
    if (!info[0]->IsObject()) {
        JS_THROW_INVALID_ARG_TYPE(NOTHING, context, info, 0, "an object");
    }
    auto object = info[0].As<v8::Object>();
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, object_context, object->GetCreationContext());
    auto security_token = object_context->GetSecurityToken();
    info.GetReturnValue().Set(security_token);
}

void js_set_security_token(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 2) {
        JS_THROW_INVALID_ARG_COUNT(NOTHING, context, info, 2);
    }
    if (!info[0]->IsObject()) {
        JS_THROW_INVALID_ARG_TYPE(NOTHING, context, info, 0, "an object");
    }
    auto object = info[0].As<v8::Object>();
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, object_context, object->GetCreationContext());
    auto security_token = object_context->GetSecurityToken();
    info.GetReturnValue().Set(security_token);
    object_context->SetSecurityToken(info[1]);
}

void js_use_default_security_token(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_INVALID_ARG_COUNT(NOTHING, context, info, 1);
    }
    if (!info[0]->IsObject()) {
        JS_THROW_INVALID_ARG_TYPE(NOTHING, context, info, 0, "an object");
    }
    auto object = info[0].As<v8::Object>();
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, object_context, object->GetCreationContext());
    auto security_token = object_context->GetSecurityToken();
    info.GetReturnValue().Set(security_token);
    object_context->UseDefaultSecurityToken();
}

void js_native_function_callback(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    v8::Local<v8::Array> call_arguments;
    auto call_this = info.This();
    auto call_target = info.NewTarget();
    {
        v8::Local<v8::Value> array_arguments[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            array_arguments[i] = info[i];
        }
        call_arguments = v8::Array::New(isolate, array_arguments, info.Length());
    }
    v8::Local<v8::Value> args[] = { call_this, call_arguments, call_target };
    v8::MaybeLocal<v8::Value> maybe_result;
    if (info.Data()->IsFunction()) {
        maybe_result = info.Data().As<v8::Function>()->Call(context, v8::Undefined(isolate), 3, args);
    } else {
        maybe_result = info.Data().As<v8::Object>()->CallAsFunction(context, v8::Undefined(isolate), 3, args);
    }
    if (maybe_result.IsEmpty()) {
        return;
    }
    info.GetReturnValue().Set(maybe_result.ToLocalChecked());
}
/**
 * @brief Create a native function in a context.
 *
 * @param info
 * @param info[0] {Function} function: Function to call when this function is invoked.
 * @param info[1] {boolean}=true constructor: Can this function be called as constructor.
 * @param info[2] {string} name: The name of the function.
 * @param info[3] {Number}=0 length: Minimum number required for the function.
 * @param info[4] {Object} context: The creation context of the function.
 */
void js_native_function(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    auto caller_context = context;
    auto is_constructor = info[1]->BooleanValue(isolate);
    JS_EXECUTE_RETURN(NOTHING, int32_t, caller_length, info[3]->Int32Value(context));
    if (!info[4]->IsNullOrUndefined()) {
        auto object = info[4].As<v8::Object>();
        auto maybe_context = object->GetCreationContext();
        if (maybe_context.IsEmpty()) {
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, message, ToDetailString(context, "Unable to retrieve the creation context for the provided object: ", object));
            isolate->ThrowException(v8::Exception::TypeError(message));
            return;
        }
        caller_context = maybe_context.ToLocalChecked();
    }
    v8::ConstructorBehavior callee_constructor_behavior = is_constructor ? v8::ConstructorBehavior::kAllow : v8::ConstructorBehavior::kThrow;
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, callee, v8::Function::New(caller_context, js_native_function_callback, info[0], caller_length, callee_constructor_behavior));
    if (info[2]->IsString()) {
        callee->SetName(info[2].As<v8::String>());
    }
    info.GetReturnValue().Set(callee);
}

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    info.GetReturnValue().SetNull();
    if (info.Length() >= 1 && info[0]->IsObject()) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, creation_context, info[0].As<v8::Object>()->GetCreationContext());
        info.GetReturnValue().Set(creation_context->Global());
    }
}

NODE_MODULE_INIT() {
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "getSecurityToken"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_get_security_token, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "setSecurityToken"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_set_security_token, exports, 2, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "useDefaultSecurityToken"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_use_default_security_token, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "nativeFunction"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_native_function, exports, 2, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "globalOf"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_global_of, exports, 5, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
}
