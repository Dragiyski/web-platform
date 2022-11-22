#ifndef NODE_EXT_API_USER_CONTEXT_HXX
#define NODE_EXT_API_USER_CONTEXT_HXX

#include <chrono>
#include <memory>
#include <optional>
#include <v8.h>
#include "../js-helper.hxx"
#include "context.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    class UserContext : public Context {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        static void prototype_get_max_user_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_set_max_user_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get_max_entry_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_set_max_entry_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_set_max_user_time(const v8::FunctionCallbackInfo<v8::Value>& info);
        // No special operation for compile_function, this cannot be reasonably protected.
        // Assume we create some sort of wrapper and never allow access to the underlying function.
        // When invoked, the wrapper enters into user stack frame block and executes the underlying function
        // (potentially with Reflect.apply, Reflect.construct) and then return the result.
        // If the result returned is an object or even worse - proxy to object, any operation:
        // including reflections (Object.getPrototypeOf) will potentially execute user code,
        // which is allowed to do `while(true);` and perform DoS attack on the NodeJS host (no timeout call).

        // On top of that returning Proxy will have signficant negative performance impact and
        // it will negate the benefit on protect-on-call interface.
        // static void prototype_compile_function(const v8::FunctionCallbackInfo<v8::Value>& info);

        // A much better option is to protect-on-call. This will accept any function, not just the compiled function,
        // and will consider execution of those as "user code" applying the time limit, even if the callee is within
        // the NodeJS context. This allows to create safe function that can access both NodeJS API and user code,
        // and examine the user-provided objects/functions. In case user code is invoked, it will be limited as part
        // of the function.
        static void prototype_apply(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_construct(const v8::FunctionCallbackInfo<v8::Value>& info);

        // Exception to this rule is Script and Module. Those are safe during creation and it will not execute
        // any user code. Unlike X.compileFunction result, which can be executed directly from JavaScript,
        // this results need to call EmbedderAPI methods "run" in order to run the script.

        // Note: Script and Module are bound to the context they are created. The Scriot and Module constructors are illegal to call.
        // Idea: Since these are wrapped to be safe, extend them to UserScript and UserModule.
        // When not bound to UserContext, the Script and Module wrapper will be simpler.
        // static void prototype_create_script(const v8::FunctionCallbackInfo<v8::Value>& info);
        // static void prototype_create_module(const v8::FunctionCallbackInfo<v8::Value>& info);
    protected:
        std::unique_ptr<v8::MicrotaskQueue> _microtask_queue;
    public:
        std::optional<std::chrono::steady_clock::duration> max_user_time;
        std::optional<std::chrono::steady_clock::duration> max_entry_time;
    protected:
        UserContext(v8::Isolate* isolate, v8::Local<v8::Context> value);
        UserContext(const Context&) = delete;
        UserContext(Context&&) = delete;
    public:
        virtual ~UserContext() override = default;
    };

    // A better idea, instead of having "apply" and "construct" directly on the UserContext,
    // allow UserContext to create interruptScope with a callback. The returned object is associated with the UserContext
    // and it will have "apply", "construct" and "execute" (for Script and Module perhaps), where the callback will be
    // invoked in case the execution is terminated within the calling function.
    // The scopes can be saved and used later.
}

#endif /* NODE_EXT_API_CONTEXT_HXX */