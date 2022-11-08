#include "security-token.h"
#include "js-helper.h"

void js_get_security_token(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
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
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
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
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
    }
    auto object = info[0].As<v8::Object>();
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, object_context, object->GetCreationContext());
    auto security_token = object_context->GetSecurityToken();
    info.GetReturnValue().Set(security_token);
    object_context->UseDefaultSecurityToken();
}
