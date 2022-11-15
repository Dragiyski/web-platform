#include "microtask-queue.h"

#include <v8.h>
#include <node.h>

#include <map>
#include <set>

#include "api-helper.h"
#include "../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    DECLARE_API_WRAPPER_BODY_MORE(MicrotaskQueue, initialize_more, uninitialize_more);

    namespace {
        // Used to associate only one JavaScript object per isolate for each microtask queue.
        // Note: the microtask queue pointer can be reused in multiple contexts.
        std::map<v8::Isolate *, std::map<v8::MicrotaskQueue *, MicrotaskQueue *>> per_isolate_holder_map;

        // Microtask queue can add a callback that is executed once for each PerformCheckout, after all microtask complete.
        // For all microtask queues associated with each context we create, those will be run when the above callback is executed.
        // Why? Because a microtask policy can only be changed on the fly for the default microtask queue.
        // All other queues receive policy on create ::New(...) and cannot retrieve, change them afterwards.
        std::map<v8::Isolate *, std::set<MicrotaskQueue *>> per_isolate_autorun_set;
        // Contains (probably single entry) if the microtask queue for the node-environment context,
        // so the microtask-completed callback can be removed from that queue.
        std::map<v8::Isolate *, std::set<v8::MicrotaskQueue *>> per_isolate_microtask_listener_set;

        void on_microtask_completed(v8::Isolate *isolate, v8::MicrotaskQueue *caller) {
            auto it = per_isolate_autorun_set.find(isolate);
            if (it != per_isolate_autorun_set.end()) {
                // Create a copy, so if any microtask somehow access the wrappers and clear the autorun flag,
                // all scheduled microtask queues still runs.
                auto autorun_set = std::set(it->second);
                for (auto &wrapper : autorun_set) {
                    wrapper->value()->PerformCheckpoint(isolate);
                }
            }
        }
    }

    v8::Maybe<void> MicrotaskQueue::initialize_more(v8::Isolate *isolate) {
        assert(per_isolate_holder_map.find(isolate) == per_isolate_holder_map.end());
        per_isolate_holder_map.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple());
        assert(per_isolate_autorun_set.find(isolate) == per_isolate_autorun_set.end());
        per_isolate_autorun_set.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple());
        return v8::JustVoid();
    }

    void MicrotaskQueue::uninitialize_more(v8::Isolate *isolate) {
        per_isolate_holder_map.erase(isolate);
        assert(per_isolate_microtask_listener_set.find(isolate) != per_isolate_microtask_listener_set.end());
        auto listener_set = per_isolate_microtask_listener_set.find(isolate)->second;
        for (auto &microtask_queue : listener_set) {
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
        return v8::JustVoid();
    }

    v8::Maybe<void> MicrotaskQueue::initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template) {
        auto signature = v8::Signature::New(isolate, class_template);
        return v8::JustVoid();
    }

    void MicrotaskQueue::get_auto_run(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        JS_EXECUTE_RETURN(NOTHING, MicrotaskQueue *, wrapper, MicrotaskQueue::unwrap(isolate, info.Holder()));
        info.GetReturnValue().Set(false);
        auto it = per_isolate_autorun_set.find(isolate);
        assert(it != per_isolate_autorun_set.end());
        auto autorun_set = it->second;
        if (autorun_set.contains(wrapper)) {
            info.GetReturnValue().Set(true);
        }
    }

    void MicrotaskQueue::set_auto_run(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        JS_EXECUTE_RETURN(NOTHING, MicrotaskQueue *, wrapper, MicrotaskQueue::unwrap(isolate, info.Data().As<v8::Object>()));
        auto flag = info[0]->BooleanValue(isolate);
        auto it = per_isolate_autorun_set.find(isolate);
        assert(it != per_isolate_autorun_set.end());
        auto autorun_set = it->second;
        if (flag) {
            autorun_set.insert(wrapper);
        } else {
            autorun_set.erase(wrapper);
        }
    }

    void MicrotaskQueue::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
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

        {
            // Set the auto property. This is only set if the MicrotaskQueue is constructed.
            // If the MicrotaskQueue is a reference, this property won't exists.
            // The user cannot control reference microqueues, as they are immutable to V8 API,
            // excluding the default microqueue of the isolate, but changes on that will affect
            // the NodeJS environment.
            JS_PROPERTY_NAME(NOTHING, name, isolate, "auto");
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, getter, v8::Function::New(
                context,
                get_auto_run,
                holder,
                0,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasNoSideEffect
            ));
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, setter, v8::Function::New(
                context,
                set_auto_run,
                holder,
                1,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasNoSideEffect
            ));
            info.This()->SetAccessorProperty(
                name,
                getter,
                setter,
                JS_PROPERTY_ATTRIBUTE_SEAL
            );
        }

        auto queue = v8::MicrotaskQueue::New(isolate, v8::MicrotasksPolicy::kExplicit);
        auto wrapper = new MicrotaskQueueWrap(isolate, std::move(queue));
        {
            auto it = per_isolate_holder_map.find(isolate);
            assert(it != per_isolate_holder_map.end());
            it->second.emplace(std::piecewise_construct, std::forward_as_tuple(wrapper->value()), std::forward_as_tuple(wrapper));
        }
        wrapper->Wrap(holder, info.This());

        info.GetReturnValue().Set(info.This());
    }

    MicrotaskQueue::MicrotaskQueue(v8::Isolate *isolate) : ObjectWrap(isolate) {}

    MicrotaskQueueWrap::MicrotaskQueueWrap(v8::Isolate *isolate, std::unique_ptr<v8::MicrotaskQueue> &&queue) :
        MicrotaskQueue(isolate),
        _queue(std::move(queue)) {}

    MicrotaskQueueWrap::~MicrotaskQueueWrap() {
        clear_holder_map();
    }

    v8::MicrotaskQueue *MicrotaskQueueWrap::value() {
        return _queue.get();
    }

    MicrotaskQueueReference::MicrotaskQueueReference(v8::Isolate *isolate, v8::MicrotaskQueue *queue) :
        MicrotaskQueue(isolate),
        _queue(queue) {}

    MicrotaskQueueReference::~MicrotaskQueueReference() {
        clear_holder_map();
    }

    v8::MicrotaskQueue *MicrotaskQueueReference::value() {
        return _queue;
    }

    // Called by the subclasses destructors prior of destructing the subclass properties,
    // as calls to value() would be invalid.
    void MicrotaskQueue::clear_holder_map() {
        auto isolate = v8::Isolate::GetCurrent();
        auto queue = value();
        auto it = per_isolate_holder_map.find(isolate);
        if (it != per_isolate_holder_map.end()) {
            auto holder_map = it->second;
            assert(holder_map.find(queue) != holder_map.end() && holder_map.find(queue)->second == this);
            holder_map.erase(queue);
        }
    }

    MicrotaskQueue::~MicrotaskQueue() {
        auto isolate = v8::Isolate::GetCurrent();
        {
            auto it = per_isolate_autorun_set.find(isolate);
            if (it != per_isolate_autorun_set.end()) {
                auto autorun_set = it->second;
                autorun_set.erase(this);
            }
        }
    }

    void MicrotaskQueue::static_get_from_current_context(const v8::FunctionCallbackInfo<v8::Value> &info) {
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
            auto wrapper = new MicrotaskQueueReference(isolate, v8_queue);
            wrapper->Wrap(holder, holder);
            holder_map.insert(std::make_pair(v8_queue, wrapper));
        }
    }
}