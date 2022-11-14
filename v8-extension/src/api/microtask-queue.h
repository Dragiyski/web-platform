#ifndef V8EXT_MICROTASK_QUEUE_H
#define V8EXT_MICROTASK_QUEUE_H

#include <memory>
#include <v8.h>
#include "api-helper.h"
#include "../object.h"

namespace dragiyski::node_ext {
    class MicrotaskQueue : public ObjectWrap {
        DECLARE_API_WRAPPER_HEAD
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        std::shared_ptr<v8::MicrotaskQueue> _queue;
        int _policy;
    protected:
        MicrotaskQueue(v8::Isolate *isolate, std::unique_ptr<v8::MicrotaskQueue> queue, int policy);
        MicrotaskQueue(const MicrotaskQueue &) = delete;
        MicrotaskQueue(MicrotaskQueue &&) = delete;
    public:
        ~MicrotaskQueue() override = default;
    };
}

#endif /* V8EXT_MICROTASK_QUEUE_H */
