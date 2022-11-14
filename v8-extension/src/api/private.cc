#include "private.h"

#include <cassert>
#include <map>
#include "../js-helper.h"

namespace dragiyski::node_ext {
    DECLARE_API_WRAPPER_BODY(Private);

    v8::Maybe<void> Private::initialize_template(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> class_template) {
        auto prototype_template = class_template->PrototypeTemplate();

        auto signature = v8::Signature::New(isolate, class_template);
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "has");
            auto func = v8::FunctionTemplate::New(isolate, prototype_has, v8::Local<v8::Value>(), signature, 1, v8::ConstructorBehavior::kThrow);
            func->SetClassName(name);
            prototype_template->Set(name, func, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "get");
            auto func = v8::FunctionTemplate::New(isolate, prototype_get, v8::Local<v8::Value>(), signature, 1, v8::ConstructorBehavior::kThrow);
            func->SetClassName(name);
            prototype_template->Set(name, func, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "set");
            auto func = v8::FunctionTemplate::New(isolate, prototype_set, v8::Local<v8::Value>(), signature, 2, v8::ConstructorBehavior::kThrow);
            func->SetClassName(name);
            prototype_template->Set(name, func, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "delete");
            auto func = v8::FunctionTemplate::New(isolate, prototype_delete, v8::Local<v8::Value>(), signature, 1, v8::ConstructorBehavior::kThrow);
            func->SetClassName(name);
            prototype_template->Set(name, func, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }

        return v8::JustVoid();
    }

    void Private::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto holder = info.This()->FindInstanceInPrototypeChain(get_template(isolate));
        if (
            !info.IsConstructCall() ||
            holder.IsEmpty() ||
            !holder->IsObject() ||
            holder.As<v8::Object>()->InternalFieldCount() < 1
            ) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal constructor");
        }

        v8::Local<v8::String> name;

        if (info.Length() >= 1 && !info[0]->IsNullOrUndefined()) {
            if (!info[0]->IsString()) {
                JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0] != null: but not a string");
            }
            name = info[0].As<v8::String>();
        }

        auto value = v8::Private::New(isolate, name);
        auto wrapper = new Private(isolate, value);
        wrapper->Wrap(holder, info.This());

        info.GetReturnValue().Set(info.This());
    }

    Private::Private(v8::Isolate* isolate, v8::Local<v8::Private> value) :
        ObjectWrap(isolate),
        _value(isolate, value) {}

    v8::Local<v8::Private> Private::value(v8::Isolate* isolate) {
        return _value.Get(isolate);
    }

    void Private::prototype_has(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        if (holder->InternalFieldCount() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal invocation");
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
        }

        auto wrapper = node::ObjectWrap::Unwrap<Private>(holder);
        auto value = wrapper->value(isolate);
        JS_EXECUTE_RETURN(NOTHING, bool, result, info[0].As<v8::Object>()->HasPrivate(context, value));
        info.GetReturnValue().Set(result);
    }

    void Private::prototype_get(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        if (holder->InternalFieldCount() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal invocation");
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
        }

        auto wrapper = node::ObjectWrap::Unwrap<Private>(holder);
        auto value = wrapper->value(isolate);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, result, info[0].As<v8::Object>()->GetPrivate(context, value));
        info.GetReturnValue().Set(result);
    }

    void Private::prototype_set(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        if (holder->InternalFieldCount() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal invocation");
        }

        if (info.Length() < 2) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 2, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
        }

        auto wrapper = node::ObjectWrap::Unwrap<Private>(holder);
        auto value = wrapper->value(isolate);
        JS_EXECUTE_RETURN(NOTHING, bool, result, info[0].As<v8::Object>()->SetPrivate(context, value, info[1]));
        info.GetReturnValue().Set(result);
    }

    void Private::prototype_delete(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        if (holder->InternalFieldCount() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal invocation");
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
        }

        auto wrapper = node::ObjectWrap::Unwrap<Private>(holder);
        auto value = wrapper->value(isolate);
        JS_EXECUTE_RETURN(NOTHING, bool, result, info[0].As<v8::Object>()->DeletePrivate(context, value));
        info.GetReturnValue().Set(result);
    }
}
