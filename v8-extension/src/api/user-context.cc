#include "user-context.h"

#include <cassert>
#include <map>
#include <set>
#include <list>
#include "../js-helper.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <ranges>
#include <variant>

#include "function-template.h"
#include "../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    DECLARE_API_WRAPPER_BODY_MORE(UserContext, initialize_more, uninitialize_more);

    namespace {
        std::map<v8::Isolate *, Shared<v8::Symbol>> per_isolate_unhandled_termination;

        struct APITimeEntry;
        struct UserTimeEntry;
        using ScheduleEntry = std::variant<APITimeEntry, UserTimeEntry>;
        using time_point = std::chrono::steady_clock::time_point;
        using duration = std::chrono::steady_clock::duration;
        struct TimeSchedule;

        std::recursive_mutex per_isolate_time_schedule_mutex;
        std::map<v8::Isolate *, std::shared_ptr<TimeSchedule>> per_isolate_time_schedule;

        std::shared_ptr<TimeSchedule> get_time_schedule(v8::Isolate *isolate) {
            std::lock_guard lock(per_isolate_time_schedule_mutex);
            auto it = per_isolate_time_schedule.find(isolate);
            assert(it != per_isolate_time_schedule.end());
            return it->second;
        }

        struct StackEntry {
            std::shared_ptr<TimeSchedule> schedule;
            StackEntry *prev, *next;
            time_point entered_at;
            duration api_duration, user_duration;

            virtual void on_stack_enter();
            virtual void on_stack_leave(duration api_duration, duration user_duration);
            virtual bool is_user_entry() = 0;

            StackEntry(std::shared_ptr<TimeSchedule> schedule, time_point entered_at = std::chrono::steady_clock::now());
            virtual ~StackEntry();
        };

        struct ApiStackEntry : public virtual StackEntry {
            duration user_duration;

            virtual inline bool is_user_entry() { return false; }

            ApiStackEntry(std::shared_ptr<TimeSchedule> schedule, time_point entered_at = std::chrono::steady_clock::now());
            virtual ~ApiStackEntry();
        };

        struct UserStackEntry : public virtual StackEntry {
            UserContext *context;
            std::optional<duration> max_entry_duration, max_user_duration;
            std::optional<time_point> max_entry_timepoint, max_user_timepoint;

            virtual void on_stack_enter();
            virtual void on_stack_leave(duration api_duration, duration user_duration);
            virtual inline bool is_user_entry() { return true; }

            UserStackEntry(std::shared_ptr<TimeSchedule> schedule, UserContext *context, time_point entered_at = std::chrono::steady_clock::now());
            virtual ~UserStackEntry() override;
        };

        struct TimeSchedule : std::enable_shared_from_this<TimeSchedule> {
            v8::Isolate *isolate;
            StackEntry *root, *top;
            std::recursive_mutex modify_mutex;
            std::mutex task_mutex;
            std::condition_variable task_notifier;
            std::thread time_thread;
            int prevent_termination = 0;
            bool has_terminated = false;

            std::multimap<time_point, UserStackEntry *> timeline;

            static void thread_function(std::shared_ptr<TimeSchedule> schedule);

            void add_to_timeline(time_point, UserStackEntry *);
            void remove_from_timeline(time_point, UserStackEntry *);

            explicit TimeSchedule(v8::Isolate *isolate) :
                std::enable_shared_from_this<TimeSchedule>(),
                isolate(isolate),
                root(nullptr),
                top(nullptr),
                modify_mutex(),
                task_mutex(),
                task_notifier(),
                time_thread(thread_function, shared_from_this()) {};

            ~TimeSchedule();
        };

        StackEntry::StackEntry(std::shared_ptr<TimeSchedule> schedule, time_point entered_at) :
            schedule(schedule),
            prev(nullptr),
            next(nullptr),
            entered_at(entered_at),
            api_duration(decltype(api_duration)::zero()),
            user_duration(decltype(user_duration)::zero()) {
            std::lock_guard lock(schedule->modify_mutex);
            if (schedule->root == nullptr) {
                assert(schedule->top == nullptr);
                schedule->root = schedule->top = this;
            } else {
                assert(schedule->top->next == nullptr);
                prev = schedule->top;
                schedule->top->next = this;
                schedule->top = this;
            }
        }

        StackEntry::~StackEntry() {
            assert(schedule->top == this);
            assert(next == nullptr);
            if (prev != nullptr) {
                prev->next = nullptr;
            } else {
                schedule->root = nullptr;
                schedule->has_terminated = false;
            }
            schedule->top = prev;
        }

        UserStackEntry::UserStackEntry(std::shared_ptr<TimeSchedule> schedule, UserContext *context, time_point entered_at) :
            StackEntry(schedule, entered_at),
            context(context),
            max_entry_duration(context->max_entry_time),
            max_user_duration(context->max_user_time) {
            std::lock_guard lock(schedule->modify_mutex);
            bool should_notify = false;
            if (max_entry_duration) {
                max_entry_timepoint = entered_at + max_entry_duration.value();
                schedule->add_to_timeline(max_entry_timepoint.value(), this);
                should_notify = true;
            }
            if (max_user_duration) {
                max_user_timepoint = entered_at + max_user_duration.value();
                schedule->add_to_timeline(max_user_timepoint.value(), this);
                should_notify = true;
            }
            if (should_notify) {
                schedule->task_notifier.notify_all();
            }
            if (prev != nullptr) {
                prev->on_stack_enter();
            }
        }

        UserStackEntry::~UserStackEntry() {
            std::lock_guard lock(schedule->modify_mutex);
            if (prev != nullptr) {
                prev->on_stack_leave(api_duration, (std::chrono::steady_clock::now() - entered_at) - api_duration);
            }
            if (max_entry_timepoint) {
                schedule->remove_from_timeline(max_entry_timepoint.value(), this);
            }
            if (max_user_timepoint) {
                schedule->remove_from_timeline(max_user_timepoint.value(), this);
            }
        }

        ApiStackEntry::ApiStackEntry(std::shared_ptr<TimeSchedule> schedule, time_point entered_at) :
            StackEntry(schedule, entered_at) {
            std::lock_guard lock(schedule->modify_mutex);
            if (prev != nullptr) {
                prev->on_stack_enter();
            }
        }

        ApiStackEntry::~ApiStackEntry() {
            std::lock_guard lock(schedule->modify_mutex);
            if (prev != nullptr) {
                prev->on_stack_leave((std::chrono::steady_clock::now() - entered_at) - user_duration, user_duration);
            }
        }

        void StackEntry::on_stack_enter() {}

        void StackEntry::on_stack_leave(duration api_duration, duration user_duration) {
            this->user_duration += user_duration;
            this->api_duration += api_duration;
        }

        void UserStackEntry::on_stack_enter() {
            StackEntry::on_stack_enter();
            if (max_user_duration.has_value() && max_user_timepoint.has_value() && !next->is_user_entry()) {
                auto timepoint = max_entry_timepoint.value();
                max_user_timepoint.reset();
                schedule->remove_from_timeline(timepoint, this);
            }
        }

        void UserStackEntry::on_stack_leave(duration api_duration, duration user_duration) {
            StackEntry::on_stack_leave(api_duration, user_duration);
            if (max_user_duration.has_value() && !next->is_user_entry()) {
                assert(!max_user_timepoint.has_value());
                max_user_timepoint = entered_at + max_user_duration.value() - this->user_duration;
                schedule->add_to_timeline(max_user_timepoint.value(), this);
            }
        }

        void TimeSchedule::add_to_timeline(time_point time, UserStackEntry *entry) {
            std::lock_guard lock(modify_mutex);
            timeline.insert(std::make_pair(time, entry));
            task_notifier.notify_all();
        }

        void TimeSchedule::remove_from_timeline(time_point time, UserStackEntry *entry) {
            std::lock_guard lock(modify_mutex);
            for (auto it = timeline.lower_bound(time); it != timeline.upper_bound(time); ++it) {
                if (it->second == entry) {
                    timeline.erase(it);
                }
            }
            task_notifier.notify_all();
        }

        void TimeSchedule::thread_function(std::shared_ptr<TimeSchedule> schedule) {
            while (true) {
                {
                    // Step 0: the schedule observer exits when isolate is disposing.
                    std::lock_guard lock(per_isolate_time_schedule_mutex);
                    if (per_isolate_time_schedule.find(schedule->isolate) == per_isolate_time_schedule.end()) {
                        return;
                    }
                }
                bool should_interrupt = false;
                std::optional<std::chrono::steady_clock::time_point> next_task_timepoint;
                std::chrono::steady_clock::time_point now;
                {
                    // Step 1: Check schedule if we have to interrupt.
                    std::lock_guard lock(schedule->modify_mutex);
                    now = std::chrono::steady_clock::now();
                    auto next_task = schedule->timeline.upper_bound(now);
                    if (next_task == schedule->timeline.end()) {
                        goto wait_for_next_task;
                    }
                    next_task_timepoint = next_task->first;
                    if (next_task == schedule->timeline.begin()) {
                        goto wait_for_next_task;
                    }
                    // This step is reached only if we need to interrupt.
                    // Handling interruptions is slow process,
                    // but it should only happen when the executed code overstep the specified time.
                    for (auto task = schedule->timeline.begin(); task != next_task; ++task) {
                        auto current_duration = now - task->second->entered_at;
                        if (task->second->max_entry_duration) {
                            auto max_duration = task->second->max_entry_duration.value();
                            if (current_duration > max_duration) {
                                should_interrupt = true;
                                break;
                            }
                        } else if (task->second->max_user_duration) {
                            auto max_duration = task->second->user_duration;
                            if (current_duration > max_duration) {
                                should_interrupt = true;
                                break;
                            }
                        }
                    }
                }
                if (should_interrupt && schedule->prevent_termination <= 0 && !schedule->has_terminated) {
                    schedule->isolate->TerminateExecution();
                    schedule->has_terminated = true;
                }
            wait_for_next_task:
                {
                    std::unique_lock lock(schedule->task_mutex);
                    if (next_task_timepoint.has_value() && next_task_timepoint.value() > now) {
                        schedule->task_notifier.wait_until(lock, next_task_timepoint.value());
                    } else {
                        schedule->task_notifier.wait(lock);
                    }
                }
            }
        }

        MaybeLocal<v8::Value> call_current_context_reflect_construct(Local<v8::Context> context, int argc, Local<v8::Value> argv[]) {
            auto isolate = context->GetIsolate();
            auto global = context->Global();
            JS_OBJECT_GET_LITERAL_KEY(JS_NOTHING(v8::Value), reflect_value, context, global, "Reflect");
            if (!reflect_value->IsObject()) {
                JS_THROW_ERROR(JS_NOTHING(v8::Value), isolate, ReferenceError, "Reflect is not defined");
            }
            auto reflect = reflect_value.As<v8::Object>();
            JS_OBJECT_GET_LITERAL_KEY(JS_NOTHING(v8::Value), reflect_construct_value, context, reflect, "construct");
            if (!reflect_construct_value->IsFunction()) {
                JS_THROW_ERROR(JS_NOTHING(v8::Value), isolate, TypeError, "Reflect.construct is not a function");
            }
            auto reflect_construct = reflect_construct_value.As<v8::Function>();
            return reflect_construct->Call(context, reflect, argc, argv);
        }
    }

    UserContext::PreventTerminationScope::PreventTerminationScope(v8::Isolate *isolate) : _isolate(isolate) {
        auto schedule = get_time_schedule(isolate);
        if V8_LIKELY(schedule) {
            schedule->prevent_termination++;
        }
    }

    UserContext::PreventTerminationScope::~PreventTerminationScope() {
        auto schedule = get_time_schedule(_isolate);
        if V8_LIKELY(schedule) {
            if V8_UNLIKELY(--schedule->prevent_termination < 0) {
                schedule->prevent_termination = 0;
            };
        }
    }

    Maybe<void> UserContext::initialize_more(v8::Isolate *isolate) {
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "UNHANDLED_TERMINATION");
            auto value = v8::Symbol::New(isolate, name);
            per_isolate_unhandled_termination.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(isolate),
                std::forward_as_tuple(isolate, value)
            );
        }
        return v8::JustVoid();
    }

    void UserContext::uninitialize_more(v8::Isolate *isolate) {
        per_isolate_unhandled_termination.erase(isolate);
    }

    v8::Local<v8::Symbol> unhandled_termination(v8::Isolate *isolate) {
        auto it = per_isolate_unhandled_termination.find(isolate);
        assert(it != per_isolate_unhandled_termination.end());
        return it->second.Get(isolate);
    }

    Maybe<void> UserContext::initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template) {
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "UNHANDLED_TERMINATION");
            class_template->Set(name, unhandled_termination(isolate), JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        auto signature = v8::Signature::New(isolate, class_template);
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "apply");
            auto value = v8::FunctionTemplate::New(
                isolate,
                secure_user_apply,
                {},
                signature,
                3,
                v8::ConstructorBehavior::kThrow
            );
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "construct");
            auto value = v8::FunctionTemplate::New(
                isolate,
                secure_user_construct,
                {},
                signature,
                2,
                v8::ConstructorBehavior::kThrow
            );
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        return v8::JustVoid();
    }

    /**
     * @brief Invoke javascript function securely.
     *
     * For the main path this is nearly identical to invoke:
     * 1. Prepare arguments;
     * 2. Make fast try_catch;
     * 3. Call the function;
     * 4. ReThrow if caught (and not terminating).
     *
     * If termination happens step 4 become:
     * 1. Cancel the pending termination immediately, so we can execute javascript;
     * 2. Try to obtain {object FunctionTemplate}.onTerminateExecution function;
     * 3. If obtained and it is a function, call it with: Call(receiver: FunctionTemplate, arguments:[callee, this, arguments, new.target])
     * 4a. If the callback return the special symbol "UNHANDLED_TERMINATION" continue the termination on the isolate.
     * 4b. Otherwise, if the callback returns a value, return that value;
     * 4c. Otherwise, if the callback throws, rethrow that exception back;
     *
     * The main process performance should be nearly identical to "invoke";
     * If termination happens, unwinding the stack might require locking the time schedule, which can be slow.
     *
     * @param info
     */
    void UserContext::secure_user_invoke(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();

        // If the protection never ran use code, using the stack is useless, as introduces a lot of work,
        // when there isn't anything waiting on the timeline.
        auto schedule = get_time_schedule(isolate);
        if (schedule->root == nullptr) {
            FunctionTemplate::invoke(info);
            return;
        }

        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Data().As<v8::Object>();
        assert(!holder.IsEmpty() && holder->IsObject() && holder.As<v8::Object>()->InternalFieldCount() >= 1);

        JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, holder));
        auto receiver = wrapper->container(isolate);
        auto callee = wrapper->callee(isolate);

        Local<v8::Value> args_list[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            args_list[i] = info[i];
        }
        auto args_array = v8::Array::New(isolate, args_list, info.Length());

        Local<v8::Value> args[] = { info.This(), args_array, info.NewTarget() };
        // Lock is obtained once termination occurs, and held until the end of this function.
        // Lock is not obtained during the user call.
        std::unique_lock<decltype(TimeSchedule::modify_mutex)> lock;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            v8::Isolate::SafeForTerminationScope safe_for_termination(isolate);
            ApiStackEntry stack_entry(schedule);
            MaybeLocal<v8::Value> return_value_maybe = callee->Call(context, receiver, 3, args);
            if (!return_value_maybe.IsEmpty()) {
                info.GetReturnValue().Set(return_value_maybe.ToLocalChecked());
                return;
            }
            if (try_catch.HasTerminated()) {
                isolate->CancelTerminateExecution();
                lock = std::unique_lock<decltype(TimeSchedule::modify_mutex)>(schedule->modify_mutex);
                goto call_termination_callback_label;
            }
            try_catch.ReThrow();
            return;
        }
    call_termination_callback_label:
        // When this is called, the schedule is locked and can be modified.
        // The protection machinery is paused, so it is not adviseable to call into the user space.
        // Any failure on any JS calls within this handles will result in resuming the termination.
        {
            v8::Local<v8::String> property_name;
            {
                auto maybe = string_map::get_string(isolate, "onTerminateExecution");
                if (maybe.IsEmpty()) {
                    isolate->TerminateExecution();
                    return;
                }
                property_name = maybe.ToLocalChecked();
            }
            v8::Local<v8::Function> termination_callback;
            {
                auto maybe = receiver->Get(context, property_name);
                if (maybe.IsEmpty()) {
                    isolate->TerminateExecution();
                    return;
                }
                auto value = maybe.ToLocalChecked();
                if (!value->IsFunction()) {
                    isolate->TerminateExecution();
                    return;
                }
                termination_callback = value.As<v8::Function>();
            }
            Local<v8::Value> args[] = { callee, info.This(), args_array, info.NewTarget() };
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            auto termination_return = termination_callback->Call(context, receiver, 4, args);
            if (!termination_return.IsEmpty()) {
                auto termination_result = termination_return.ToLocalChecked();
                auto unhandled = unhandled_termination(isolate);
                if (termination_result->SameValue(unhandled)) {
                    // Continue the termination as requested by the user.
                    isolate->TerminateExecution();
                    return;
                }
                info.GetReturnValue().Set(termination_return.ToLocalChecked());
                schedule->has_terminated = false;
                return;
            }
            if (!try_catch.HasTerminated()) {
                schedule->has_terminated = false;
            }
            try_catch.ReThrow();
        }
    }

    void UserContext::secure_user_apply(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        auto schedule = get_time_schedule(isolate);
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        // arguments: [target, self, args]
        if (info.Length() < 3) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 3, " arguments, got ", info.Length());
        }
        if (!info[0]->IsFunction()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected arguments[0] to be a function.");
        }
        if (!info[2]->IsArray()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected arguments[2] to be an array.");
        }

        auto holder = info.Holder();
        JS_EXECUTE_RETURN(NOTHING, UserContext *, wrapper, UserContext::unwrap(isolate, holder));
        auto receiver = wrapper->container(isolate);

        auto callee = info[0].As<v8::Function>();
        auto args = info[2].As<v8::Array>();
        Local<v8::Value> args_list[args->Length()];
        for (decltype(args->Length()) i = 0; i < args->Length(); ++i) {
            args_list[i] = info[i];
        }

        std::unique_lock<decltype(TimeSchedule::modify_mutex)> lock;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            v8::Isolate::SafeForTerminationScope safe_for_termination(isolate);
            UserStackEntry stack_entry(schedule, wrapper);
            auto maybe_return_value = callee->Call(wrapper->value(isolate), info[1], args->Length(), args_list);
            if (!maybe_return_value.IsEmpty()) {
                info.GetReturnValue().Set(maybe_return_value.ToLocalChecked());
                return;
            }
            if (try_catch.HasTerminated()) {
                isolate->CancelTerminateExecution();
                lock = std::unique_lock<decltype(TimeSchedule::modify_mutex)>(schedule->modify_mutex);
                goto call_termination_callback_label;
            }
            try_catch.ReThrow();
            return;
        }
    call_termination_callback_label:
        {
            v8::Local<v8::String> property_name;
            {
                auto maybe = string_map::get_string(isolate, "onTerminateExecution");
                if (maybe.IsEmpty()) {
                    isolate->TerminateExecution();
                    return;
                }
                property_name = maybe.ToLocalChecked();
            }
            v8::Local<v8::Function> termination_callback;
            {
                auto maybe = receiver->Get(context, property_name);
                if (maybe.IsEmpty()) {
                    isolate->TerminateExecution();
                    return;
                }
                auto value = maybe.ToLocalChecked();
                if (!value->IsFunction()) {
                    isolate->TerminateExecution();
                    return;
                }
                termination_callback = value.As<v8::Function>();
            }
            auto args_array = v8::Array::New(isolate, args_list, info.Length());
            Local<v8::Value> args[] = { callee, info.This(), args_array, info.NewTarget() };
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            auto termination_return = termination_callback->Call(context, receiver, 4, args);
            if (!termination_return.IsEmpty()) {
                auto termination_result = termination_return.ToLocalChecked();
                auto unhandled = unhandled_termination(isolate);
                if (termination_result->SameValue(unhandled)) {
                    // Continue the termination as requested by the user.
                    isolate->TerminateExecution();
                    return;
                }
                info.GetReturnValue().Set(termination_return.ToLocalChecked());
                schedule->has_terminated = false;
                return;
            }
            if (!try_catch.HasTerminated()) {
                schedule->has_terminated = false;
            }
            try_catch.ReThrow();
        }
    }

    void UserContext::secure_user_construct(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        auto schedule = get_time_schedule(isolate);
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        // arguments: [target, args, newTarget]
        if (info.Length() < 2) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 2, " arguments, got ", info.Length());
        }
        if (!info[0]->IsFunction()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected arguments[0] to be a function.");
        }
        if (!info[1]->IsArray()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected arguments[1] to be an array.");
        }
        v8::Local<v8::Function> newTarget;
        if (!info[2]->IsNullOrUndefined()) {
            if (!info[2]->IsFunction()) {
                JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected arguments[2] to be a function.");
            }
            newTarget = info[2].As<v8::Function>();
        }

        auto holder = info.Holder();
        JS_EXECUTE_RETURN(NOTHING, UserContext *, wrapper, UserContext::unwrap(isolate, holder));
        auto receiver = wrapper->container(isolate);

        auto callee = info[0].As<v8::Function>();
        auto args = info[2].As<v8::Array>();
        Local<v8::Value> args_list[args->Length()];
        for (decltype(args->Length()) i = 0; i < args->Length(); ++i) {
            args_list[i] = info[i];
        }

        std::unique_lock<decltype(TimeSchedule::modify_mutex)> lock;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            v8::Isolate::SafeForTerminationScope safe_for_termination(isolate);
            UserStackEntry stack_entry(schedule, wrapper);
            MaybeLocal<v8::Value> maybe_return_value;
            if (newTarget.IsEmpty()) {
                auto retval = callee->NewInstance(wrapper->value(isolate), args->Length(), args_list);
                if (!retval.IsEmpty()) {
                    maybe_return_value = retval.ToLocalChecked().As<v8::Value>();
                }
            } else {
                // The current V8 API provide no
                Local<v8::Value> reflect_args[] = { callee, info[1], newTarget };
                maybe_return_value = call_current_context_reflect_construct(context, 3, reflect_args);
            }
            if (!maybe_return_value.IsEmpty()) {
                info.GetReturnValue().Set(maybe_return_value.ToLocalChecked());
                return;
            }
            if (try_catch.HasTerminated()) {
                isolate->CancelTerminateExecution();
                lock = std::unique_lock<decltype(TimeSchedule::modify_mutex)>(schedule->modify_mutex);
                goto call_termination_callback_label;
            }
            try_catch.ReThrow();
            return;
        }
    call_termination_callback_label:
        {
            v8::Local<v8::String> property_name;
            {
                auto maybe = string_map::get_string(isolate, "onTerminateExecution");
                if (maybe.IsEmpty()) {
                    isolate->TerminateExecution();
                    return;
                }
                property_name = maybe.ToLocalChecked();
            }
            v8::Local<v8::Function> termination_callback;
            {
                auto maybe = receiver->Get(context, property_name);
                if (maybe.IsEmpty()) {
                    isolate->TerminateExecution();
                    return;
                }
                auto value = maybe.ToLocalChecked();
                if (!value->IsFunction()) {
                    isolate->TerminateExecution();
                    return;
                }
                termination_callback = value.As<v8::Function>();
            }
            auto args_array = v8::Array::New(isolate, args_list, info.Length());
            Local<v8::Value> args[] = { callee, info.This(), args_array, info.NewTarget() };
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            auto termination_return = termination_callback->Call(context, receiver, 4, args);
            if (!termination_return.IsEmpty()) {
                auto termination_result = termination_return.ToLocalChecked();
                auto unhandled = unhandled_termination(isolate);
                if (termination_result->SameValue(unhandled)) {
                    // Continue the termination as requested by the user.
                    isolate->TerminateExecution();
                    return;
                }
                info.GetReturnValue().Set(termination_return.ToLocalChecked());
                schedule->has_terminated = false;
                return;
            }
            if (!try_catch.HasTerminated()) {
                schedule->has_terminated = false;
            }
            try_catch.ReThrow();
        }
    }
}
