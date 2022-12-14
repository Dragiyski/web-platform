#ifndef V8EXT_API_USER_CONTEXT_H
#define V8EXT_API_USER_CONTEXT_H

#include <memory>
#include <v8.h>
#include "api-helper.h"
#include "../object.h"
#include "context.h"
#include <chrono>
#include <optional>

namespace dragiyski::node_ext {
    using namespace v8_handles;
    class UserContext : public Context {
        DECLARE_API_WRAPPER_HEAD(UserContext);
    protected:
        static v8::Maybe<void> initialize_more(v8::Isolate* isolate);
        static void uninitialize_more(v8::Isolate* isolate);
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        static Local<v8::Symbol> unhandled_termination(v8::Isolate* isolate);
        static void secure_user_invoke(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void secure_user_apply(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void secure_user_construct(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        static void get_max_user_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void set_max_user_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void get_max_entry_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void set_max_entry_time(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        std::shared_ptr<v8::MicrotaskQueue> _microtask_queue;
    public:
        std::optional<std::chrono::steady_clock::duration> max_user_time;
        std::optional<std::chrono::steady_clock::duration> max_entry_time;
    public:
        std::shared_ptr<v8::MicrotaskQueue> microtask_queue() const;
    protected:
        UserContext(v8::Isolate* isolate, Local<v8::Context> context, std::unique_ptr<v8::MicrotaskQueue> microtask_queue);
        UserContext(const Context&) = delete;
        UserContext(Context&&) = delete;
    public:
        ~UserContext() override = default;

        class PreventTerminationScope {
            v8::Isolate* _isolate;
        public:
            PreventTerminationScope(v8::Isolate* isolate);
            ~PreventTerminationScope();
        };
    };
}

#endif /* V8EXT_API_USER_CONTEXT_H */