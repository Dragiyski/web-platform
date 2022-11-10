#ifndef V8EXT_CONTEXT_H
#define V8EXT_CONTEXT_H

#include <v8.h>
#include <node_object_wrap.h>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#include <chrono>

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info);

class Context : node::ObjectWrap {
public:
    static v8::MaybeLocal<v8::FunctionTemplate> Template(v8::Local<v8::Context> context);
    static v8::MaybeLocal<v8::Private> Symbol(v8::Local<v8::Context> context);
protected:
    static void Constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void NativeFunction(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void creation_context_after_microtasks_completed(v8::Isolate *, void *);
protected:
    v8::Isolate *m_isolate;
    v8::Persistent<v8::Context> m_context;
    v8::Persistent<v8::Context> m_control_context;
    std::unique_ptr<v8::MicrotaskQueue> m_task_queue;
    bool auto_run_microtask_queue = false;
    std::optional<std::chrono::steady_clock::duration> m_max_sync_time;
    std::optional<std::chrono::steady_clock::duration> m_max_user_time;
protected:
    explicit Context(v8::Local<v8::Context> control_context, v8::Local<v8::Context> wrap_context, std::unique_ptr<v8::MicrotaskQueue> &&task_queue);
public:
    ~Context();
};

#endif /* V8EXT_CONTEXT_H */
