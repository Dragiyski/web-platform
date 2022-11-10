#ifndef V8EXT_CONTEXT_H
#define V8EXT_CONTEXT_H

#include <v8.h>
#include <map>
#include <thread>

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info);

void js_create_context(const v8::FunctionCallbackInfo<v8::Value> &info);

class Context {
private:
    static std::map<v8::Isolate *, v8::Global<v8::FunctionTemplate>> class_template;
public:
    static v8::MaybeLocal<v8::FunctionTemplate> Template(v8::Local<v8::Context> context);
protected:
    static void Constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void callback_promise_init_hook(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void callback_promise_before_hook(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void callback_promise_after_hook(const v8::FunctionCallbackInfo<v8::Value> &info);
    static void callback_promise_resolve_hook(const v8::FunctionCallbackInfo<v8::Value> &info);
protected:
    v8::Persistent<v8::Context> m_context;
    v8::Persistent<v8::Set> m_promise_init_hook, m_promise_before_hook, m_promise_after_hook, m_promise_resolve_hook;
    // std::unique_ptr<std::jthread>
};

#endif /* V8EXT_CONTEXT_H */
