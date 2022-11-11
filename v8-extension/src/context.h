#ifndef V8EXT_CONTEXT_H
#define V8EXT_CONTEXT_H

#include <v8.h>
#include <node_object_wrap.h>
#include <map>
#include <mutex>
#include <condition_variable>
#include <set>
#include <thread>
#include <optional>
#include <chrono>

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info);

class Context : public node::ObjectWrap {
    typedef struct per_isolate_value {
        v8::Global<v8::FunctionTemplate> class_template;
        v8::Global<v8::Private> class_symbol;
        std::recursive_mutex active_context_lock;
        std::set<Context *> active_context;

        per_isolate_value() = default;
        per_isolate_value(const per_isolate_value &) = delete;
    } per_isolate_value;
    static std::map<v8::Isolate *, per_isolate_value> per_isolate;
public:
    static v8::Maybe<void> Init(v8::Local<v8::Context> context);
    static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate *isolate);
    static v8::Local<v8::Private> get_symbol(v8::Isolate *isolate);
protected:
    static void throw_illegal_constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
    static v8::Maybe<void> init_template(v8::Local<v8::Context> context);
    static v8::Maybe<void> init_symbol(v8::Local<v8::Context> context);
    static void at_exit(v8::Isolate *isolate);
    static void on_isolate_dispose(v8::Isolate *isolate);
    static void Constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void NativeFunction(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void CompileFunction(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void Dispose(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void GetGlobal(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);
    static void creation_context_after_microtasks_completed(v8::Isolate *, Context *);
    static bool access_check(v8::Local<v8::Context> accessing_context, v8::Local<v8::Object> accessed_object, v8::Local<v8::Value> data);
    static void timer_thread_function(Context *wrap);
    static void native_function_callback(const v8::FunctionCallbackInfo<v8::Value> &info);
protected:
    v8::Isolate *m_isolate;
    v8::Persistent<v8::Context> m_control_context;
    v8::Persistent<v8::Context> m_context;
    std::unique_ptr<v8::MicrotaskQueue> m_task_queue;
    bool auto_run_microtask_queue;
protected:
    explicit Context(v8::Local<v8::Context> control_context, v8::Local<v8::Context> wrap_context, std::unique_ptr<v8::MicrotaskQueue> &&task_queue);
public:
    virtual ~Context();
};

#endif /* V8EXT_CONTEXT_H */
