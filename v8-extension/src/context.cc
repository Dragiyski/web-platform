#include "context.h"
#include "js-helper.h"

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    info.GetReturnValue().SetNull();
    if (info.Length() >= 1 && info[0]->IsObject()) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, creation_context, info[0].As<v8::Object>()->GetCreationContext());
        info.GetReturnValue().Set(creation_context->Global());
    }
}

bool v8_context_access_check(v8::Local<v8::Context> accessing_context, v8::Local<v8::Object> accessed_object, v8::Local<v8::Value> value) {
    return accessing_context->Global()->SameValue(value);
}

void js_create_context(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto current_context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsString()) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
    }
    auto name = info[0].As<v8::String>();
    auto current_global = current_context->Global();
    auto class_template = v8::FunctionTemplate::New(isolate);
    class_template->SetClassName(name);
    // WARNING: Don't install access checks on the prototype template, as it breaks V8 DCHECK
    class_template->InstanceTemplate()->SetAccessCheckCallback(v8_context_access_check, current_global);
    // The new context MUST be created with the current context microtask queue, as all executions
    // in the new context are synchronous with the current context.
    auto context = v8::Context::New(
        isolate,
        nullptr,
        class_template->InstanceTemplate(),
        v8::Local<v8::Value>(),
        v8::DeserializeInternalFieldsCallback(),
        current_context->GetMicrotaskQueue()
    );
    info.GetReturnValue().Set(context->Global());
}
