#ifndef V8EXT_API_MICROTASK_QUEUE_H
#define V8EXT_API_MICROTASK_QUEUE_H

#include <memory>
#include <v8.h>
#include "api-helper.h"
#include "../object.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    class MicrotaskQueue : public ObjectWrap {
        DECLARE_API_WRAPPER_HEAD(MicrotaskQueue)
        static v8::Maybe<void> initialize_more(v8::Isolate *isolate);
        static void uninitialize_more(v8::Isolate *isolate);
        static v8::Maybe<void> initialize(Local<v8::Context> context);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void static_get_from_current_context(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void get_auto_run(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void set_auto_run(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        virtual v8::MicrotaskQueue* value() = 0;
    protected:
        MicrotaskQueue(v8::Isolate *isolate);
        MicrotaskQueue(const MicrotaskQueue &) = delete;
        MicrotaskQueue(MicrotaskQueue &&) = delete;
    protected:
        void clear_holder_map();
    public:
        virtual ~MicrotaskQueue();
    };

    class MicrotaskQueueWrap : public MicrotaskQueue {
    protected:
        std::unique_ptr<v8::MicrotaskQueue> _queue;
    public:
        virtual v8::MicrotaskQueue* value();
    public:
        MicrotaskQueueWrap(v8::Isolate *isolate, std::unique_ptr<v8::MicrotaskQueue> &&queue);
    protected:
        MicrotaskQueueWrap(const MicrotaskQueueWrap &) = delete;
        MicrotaskQueueWrap(MicrotaskQueueWrap &&) = delete;
    public:
        virtual ~MicrotaskQueueWrap();
    };

    class MicrotaskQueueReference : public MicrotaskQueue {
    protected:
        v8::MicrotaskQueue *_queue;
    public:
        virtual v8::MicrotaskQueue* value();
    public:
        MicrotaskQueueReference(v8::Isolate *isolate, v8::MicrotaskQueue *queue);
    protected:
        MicrotaskQueueReference(const MicrotaskQueueReference &) = delete;
        MicrotaskQueueReference(MicrotaskQueueReference &&) = delete;
    public:
        virtual ~MicrotaskQueueReference() override;
    };
}

#endif /* V8EXT_API_MICROTASK_QUEUE_H */
