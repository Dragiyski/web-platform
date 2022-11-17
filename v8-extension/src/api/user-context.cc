#include "user-context.h"

#include <cassert>
#include <map>
#include <set>
#include <list>
#include "../js-helper.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <ranges>
#include <variant>

#include "function-template.h"
#include "object-template.h"
#include "../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    DECLARE_API_WRAPPER_BODY_MORE(UserContext, initialize_more, uninitialize_more);

    namespace {
        std::map<v8::Isolate*, Shared<v8::Symbol>> per_isolate_unhandled_termination;
    }

    Maybe<void> UserContext::initialize_more(v8::Isolate* isolate) {
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "UNHANDLED_TERMINATION");
            auto value = v8::Symbol::New(isolate, name);
            per_isolate_unhandled_termination.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(isolate),
                std::forward_as_tuple(isolate, value)
            );
        }
        return v8::JustVoid();
    }

    void UserContext::uninitialize_more(v8::Isolate* isolate) {
        per_isolate_unhandled_termination.erase(isolate);
    }

    Local<v8::Symbol> UserContext::unhandled_termination(v8::Isolate* isolate) {
        auto it = per_isolate_unhandled_termination.find(isolate);
        assert(it != per_isolate_unhandled_termination.end());
        return it->second.Get(isolate);
    }

    Maybe<void> UserContext::initialize_template(v8::Isolate* isolate, Local<v8::FunctionTemplate> class_template) {
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "UNHANDLED_TERMINATION");
            class_template->Set(name, unhandled_termination(isolate), JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        auto signature = v8::Signature::New(isolate, class_template);
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "apply");
            auto value = v8::FunctionTemplate::New(
                isolate,
                secure_user_apply,
                {},
                signature,
                3,
                v8::ConstructorBehavior::kThrow
            );
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "construct");
            auto value = v8::FunctionTemplate::New(
                isolate,
                secure_user_construct,
                {},
                signature,
                2,
                v8::ConstructorBehavior::kThrow
            );
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        return v8::JustVoid();
    }

    void UserContext::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

        v8::Local<v8::ObjectTemplate> global_template;
        if (info.Length() >= 1 && !info[0]->IsNullOrUndefined()) {
            if (!info[0]->IsObject()) {
                JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
            }
            auto receiver = info[0].As<v8::Object>();
            JS_EXECUTE_RETURN(NOTHING, ObjectTemplate*, wrapper, ObjectTemplate::unwrap(isolate, receiver));
            global_template = wrapper->this_object_template(isolate);
        }

        auto microtask_queue = v8::MicrotaskQueue::New(isolate, v8::MicrotasksPolicy::kExplicit);

        auto new_context = v8::Context::New(
            isolate,
            nullptr,
            global_template,
            {},
            v8::DeserializeEmbedderFieldsCallback(),
            microtask_queue.get()
        );

        auto wrapper = new UserContext(isolate, new_context, std::move(microtask_queue));
        wrapper->Wrap(holder, info.This());

        info.GetReturnValue().Set(info.This());
    }

    UserContext::UserContext(v8::Isolate* isolate, Local<v8::Context> context, std::unique_ptr<v8::MicrotaskQueue> microtask_queue) :
        Context(isolate, context),
        _microtask_queue(std::move(microtask_queue)) {}
}
