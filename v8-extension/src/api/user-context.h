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
    protected:
        std::optional<std::chrono::steady_clock::duration> max_user_time;
        std::optional<std::chrono::steady_clock::duration> max_entry_time;
    protected:
        UserContext(const Context&) = delete;
        UserContext(Context&&) = delete;
    public:
        ~UserContext() override = default;
    };
}

#endif /* V8EXT_API_USER_CONTEXT_H */