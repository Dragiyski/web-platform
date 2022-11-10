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

std::map<v8::Isolate *, v8::Global<v8::FunctionTemplate>> Context::class_template;

void v8_throw_illegal_constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();

    JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
}

v8::MaybeLocal<v8::FunctionTemplate> Context::Template(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    auto location = class_template.find(isolate);
    if (location != class_template.end()) {
        auto result = location->second.Get(isolate);
        return result;
    }
    auto tpl = v8::FunctionTemplate::New(isolate, Constructor, v8::Local<v8::Value>(), v8::Local<v8::Signature>());
    JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::String, name, ToString(context, "Context"));
    tpl->SetClassName(name);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    class_template.insert(std::make_pair(isolate, v8::Global<v8::FunctionTemplate>(isolate, tpl)));
    return tpl;
}

void Context::Constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (!info.IsConstructCall()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }

    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::FunctionTemplate, context_template, Template(context));

    auto holder = info.This()->FindInstanceInPrototypeChain(context_template);
    if (holder.IsEmpty() || holder->InternalFieldCount() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }

    v8::Local<v8::Object> options;
    if (info.Length() < 1) {
        options = v8::Object::New(isolate);
    } else if (!info[0]->IsNullOrUndefined()) {
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, if specified");
        }
        options = info[0].As<v8::Object>();
    }

    v8::Local<v8::String> context_name;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::String, js_value, context, options, "name");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'name' to be a string");
            }
            context_name = js_value;
        }
    }

    auto global_constructor_template = v8::FunctionTemplate::New(isolate, v8_throw_illegal_constructor, holder);
    if (!context_name.IsEmpty()) {
        global_constructor_template->SetClassName(context_name);
    }

    auto new_context = v8::Context::New(
        isolate,
        nullptr,
        global_constructor_template->InstanceTemplate(),
        v8::MaybeLocal<v8::Value>(),
        v8::DeserializeInternalFieldsCallback(),
        context->GetMicrotaskQueue()
    );

    auto promise_init_hook = v8::Set::New(isolate);
    auto promise_before_hook = v8::Set::New(isolate);
    auto promise_after_hook = v8::Set::New(isolate);
    auto promise_resolve_hook = v8::Set::New(isolate);
}
