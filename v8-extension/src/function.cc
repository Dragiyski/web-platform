#include "function.h"
#include "js-helper.h"

void js_function_set_name(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 2) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
    }
    if (!info[0]->IsFunction()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
    }
    if (!info[1]->IsString()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[1] to be a string, got ", info[1]);
    }
    info[0].As<v8::Function>()->SetName(info[1].As<v8::String>());
}

void js_function_get_name(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
    }
    if (!info[0]->IsFunction()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
    }
    info.GetReturnValue().Set(info[0].As<v8::Function>()->GetName());
}
