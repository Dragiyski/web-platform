#include "microtask-queue.h"

#include <v8.h>
#include <node.h>

#include <map>
#include <set>

#include "api-helper.h"

namespace dragiyski::node_ext {
    DECLARE_API_WRAPPER_BODY_MORE(MicrotaskQueue, initialize_more, uninitialize_more);

    namespace {
        std::map<v8::Isolate*, std::map<v8::MicrotaskQueue*, MicrotaskQueue*>> per_isolate_holder_map;
        std::map<v8::Isolate*, std::set<MicrotaskQueue*>> per_isolate_autorun_set;
        std::map<v8::Isolate*, std::set<v8::MicrotaskQueue*>> per_isolate_microtask_listener_set;

        void on_microtask_completed(v8::Isolate* isolate, v8::MicrotaskQueue* caller) {
            auto it = per_isolate_autorun_set.find(isolate);
            if (it != per_isolate_autorun_set.end()) {
                // Create a copy, so if any microtask somehow access the wrappers and clear the autorun flag,
                // all scheduled microtask queues still runs.
                auto autorun_set = std::set(it->second);
                for (auto& wrapper : autorun_set) {
                    wrapper->queue()->PerformCheckpoint(isolate);
                }
            }
        }
    }

    v8::Maybe<void> MicrotaskQueue::initialize_more(v8::Isolate* isolate) {
        assert(per_isolate_holder_map.find(isolate) == per_isolate_holder_map.end());
        per_isolate_holder_map.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple());

        return v8::JustVoid();
    }

    void MicrotaskQueue::uninitialize_more(v8::Isolate* isolate) {
        per_isolate_holder_map.erase(isolate);
        assert(per_isolate_microtask_listener_set.find(isolate) != per_isolate_microtask_listener_set.end());
        auto listener_set = per_isolate_microtask_listener_set.find(isolate)->second;
        for (auto& microtask_queue : listener_set) {
            microtask_queue->RemoveMicrotasksCompletedCallback(
                reinterpret_cast<v8::MicrotasksCompletedCallbackWithData>(on_microtask_completed),
                microtask_queue
            );
        }
    }

    v8::Maybe<void> MicrotaskQueue::initialize(v8::Local<v8::Context> context) {
        auto isolate = context->GetIsolate();
        JS_EXECUTE_IGNORE(VOID_NOTHING, initialize(isolate));
        auto insert = per_isolate_microtask_listener_set.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple());
        auto listener_set = insert.first->second;
        auto microtask_queue = context->GetMicrotaskQueue();
        if (!listener_set.contains(microtask_queue)) {
            microtask_queue->AddMicrotasksCompletedCallback(
                reinterpret_cast<v8::MicrotasksCompletedCallbackWithData>(on_microtask_completed),
                microtask_queue
            );
        }
    }

    v8::Maybe<void> MicrotaskQueue::initialize_template(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> class_template) {
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
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "UNKNOWN");
            auto value = v8::Integer::New(isolate, -1);
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
        }
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "AFTER_MICROTASKS");
            auto value = v8::Integer::New(isolate, -100);
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT);
        }
        return v8::JustVoid();
    }

    void get_auto_run(v8::Local<v8::Name> property, const v8::PropertyCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        JS_EXECUTE_RETURN(NOTHING, MicrotaskQueue*, wrapper, MicrotaskQueue::unwrap(isolate, info.Data().As<v8::Object>()));
        info.GetReturnValue().Set(false);
        auto it = per_isolate_autorun_set.find(isolate);
        if (it != per_isolate_autorun_set.end()) {
            auto autorun_set = it->second;
            if (autorun_set.contains(wrapper)) {
                info.GetReturnValue().Set(true);
            }
        }
    }

    void set_auto_run(v8::Local<v8::Name> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info) {
        auto isolate = info.GetIsolate();
        JS_EXECUTE_RETURN(NOTHING, MicrotaskQueue*, wrapper, MicrotaskQueue::unwrap(isolate, info.Data().As<v8::Object>()));
        auto flag = value->BooleanValue(isolate);
        auto it = per_isolate_autorun_set.find(isolate);
        if (it != per_isolate_autorun_set.end()) {
            auto autorun_set = it->second;
            if (flag) {
                autorun_set.insert(wrapper);
            } else {
                autorun_set.erase(wrapper);
            }
        }
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

        auto queue = v8::MicrotaskQueue::New(isolate, v8::MicrotasksPolicy::kExplicit);
        auto wrapper = new MicrotaskQueue(isolate, std::move(queue));
        {
            auto it = per_isolate_holder_map.find(isolate);
            assert(it != per_isolate_holder_map.end());
            it->second.emplace(std::piecewise_construct, std::forward_as_tuple(queue.get()), std::forward_as_tuple(wrapper));
        }
        wrapper->Wrap(holder, info.This());

        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "auto");
            JS_EXECUTE_IGNORE(NOTHING, info.This()->SetAccessor(
                context,
                name,
                get_auto_run,
                set_auto_run,
                holder,
                v8::AccessControl::ALL_CAN_READ,
                JS_PROPERTY_ATTRIBUTE_FROZEN,
                v8::SideEffectType::kHasNoSideEffect
            ));
        }

        info.GetReturnValue().Set(info.This());
    }

    MicrotaskQueue::MicrotaskQueue(v8::Isolate* isolate, std::unique_ptr<v8::MicrotaskQueue> queue) :
        ObjectWrap(isolate),
        _isolate(isolate),
        _queue(queue.release()),
        _owned(true) {
        }

    MicrotaskQueue::MicrotaskQueue(v8::Isolate* isolate, v8::MicrotaskQueue* queue) :
        ObjectWrap(isolate),
        _isolate(isolate),
        _queue(queue),
        _owned(false) {
        }

    MicrotaskQueue::~MicrotaskQueue() {
        auto isolate = _isolate;
        {
            auto it = per_isolate_holder_map.find(isolate);
            if (it != per_isolate_holder_map.end()) {
                auto holder_map = it->second;
                assert(holder_map.find(_queue) != holder_map.end() && holder_map.find(_queue)->second == this);
                holder_map.erase(_queue);
            }
        }
        {
            auto it = per_isolate_autorun_set.find(isolate);
            if (it != per_isolate_autorun_set.end()) {
                auto autorun_set = it->second;
                autorun_set.erase(this);
            }
        }
        if (_owned) {
            delete _queue;
        }
    }

    void MicrotaskQueue::static_get_from_current_context(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto v8_queue = context->GetMicrotaskQueue();
        assert(per_isolate_holder_map.find(isolate) != per_isolate_holder_map.end());
        auto holder_map = per_isolate_holder_map.find(isolate)->second;
        auto it = holder_map.find(v8_queue);
        if (it != holder_map.end()) {
            auto self = it->second->container(isolate);
            info.GetReturnValue().Set(self);
        } else {
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, holder, get_template(isolate)->InstanceTemplate()->NewInstance(context));
            auto wrapper = new MicrotaskQueue(isolate, v8_queue);
            wrapper->Wrap(holder, holder);
            holder_map.insert(std::make_pair(v8_queue, wrapper));
        }
    }
}