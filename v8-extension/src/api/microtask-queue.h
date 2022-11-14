#ifndef V8EXT_MICROTASK_QUEUE_H
#define V8EXT_MICROTASK_QUEUE_H

#include <memory>
#include <v8.h>
#include "api-helper.h"
#include "../object.h"

namespace dragiyski::node_ext {
    class MicrotaskQueue : public ObjectWrap {
        DECLARE_API_WRAPPER_HEAD(MicrotaskQueue)
        static v8::Maybe<void> initialize_more(v8::Isolate *isolate);
        static void uninitialize_more(v8::Isolate *isolate);
        static v8::Maybe<void> initialize(v8::Local<v8::Context> context);
        enum class Policy {
            kExplicit = static_cast<int>(v8::MicrotasksPolicy::kExplicit),
            kScoped = static_cast<int>(v8::MicrotasksPolicy::kScoped),
            kAuto = static_cast<int>(v8::MicrotasksPolicy::kAuto),
            kUnknown = -1,
            kAfterThis = -100
        };
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void static_get_from_current_context(const v8::FunctionCallbackInfo<v8::Value>& info);
    private:
        v8::Isolate *_isolate;
    protected:
        v8::MicrotaskQueue* _queue;
        bool _owned;
    public:
        v8::MicrotaskQueue* queue();
    protected:
        MicrotaskQueue(v8::Isolate *isolate, std::unique_ptr<v8::MicrotaskQueue> queue);
        MicrotaskQueue(v8::Isolate *isolate, v8::MicrotaskQueue* queue);
        MicrotaskQueue(const MicrotaskQueue &) = delete;
        MicrotaskQueue(MicrotaskQueue &&) = delete;
    public:
        virtual ~MicrotaskQueue();
    };
}

#endif /* V8EXT_MICROTASK_QUEUE_H */
