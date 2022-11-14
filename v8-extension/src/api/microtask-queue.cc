#include "microtask-queue.h"

#include <v8.h>
#include "api-helper.h"

namespace dragiyski::node_ext {
    v8::Maybe<void> MicrotaskQueue::initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template) {
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "EXPLICIT");
            auto value = v8::Integer::New(isolate, static_cast<int>(v8::MicrotasksPolicy::kExplicit));
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
        }
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "AUTO");
            auto value = v8::Integer::New(isolate, static_cast<int>(v8::MicrotasksPolicy::kAuto));
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
        }
        return v8::JustVoid();
    }

    void MicrotaskQueue::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.This()->FindInstanceInPrototypeChain(get_template(isolate));
        if (
            !info.IsConstructCall() ||
            holder.IsEmpty() ||
            !holder->IsObject() ||
            holder.As<v8::Object>()->InternalFieldCount() < 1
            ) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal constructor");
        }

        auto policy = v8::MicrotasksPolicy::kExplicit;
        int policy_value = static_cast<int>(policy);
        if (info.Length() >= 1 && !info[0]->IsNullOrUndefined()) {
            JS_EXECUTE_RETURN(NOTHING, int, value, info[0]->Int32Value(context));
            if (value == static_cast<int>(v8::MicrotasksPolicy::kExplicit)) {
                policy = v8::MicrotasksPolicy::kExplicit;
            } else if (value == static_cast<int>(v8::MicrotasksPolicy::kAuto)) {
                policy = v8::MicrotasksPolicy::kAuto;
            } else {
                JS_THROW_ERROR(NOTHING, context, RangeError, "Invalid microtask policy: ", info[0]);
            }
            policy_value = value;
        }

        auto queue = v8::MicrotaskQueue::New(isolate, policy);
        auto wrapper = new MicrotaskQueue(isolate, std::move(queue), policy_value);
        wrapper->Wrap(holder);

        info.GetReturnValue().Set(info.This());
    }

    MicrotaskQueue::MicrotaskQueue(v8::Isolate *isolate, std::unique_ptr<v8::MicrotaskQueue> queue, int policy) :
        ObjectWrap(isolate),
        _queue(std::move(queue)),
        _policy(policy) {}
}