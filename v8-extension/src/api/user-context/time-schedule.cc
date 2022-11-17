#include "time-schedule.h"

namespace dragiyski::node_ext::user_code {
    using namespace v8_handles;

    namespace {
        std::recursive_mutex per_isolate_time_schedule_mutex;
        std::map<v8::Isolate*, std::shared_ptr<TimeSchedule>> per_isolate_time_schedule;
    }

    TimeSchedule::TimeSchedule(v8::Isolate* isolate) :
        std::enable_shared_from_this<TimeSchedule>(),
        isolate(isolate),
        root(nullptr),
        top(nullptr),
        modify_mutex(),
        task_mutex(),
        task_notifier(),
        time_thread(thread_function, shared_from_this()) {};

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

    UserStackEntry::UserStackEntry(std::shared_ptr<TimeSchedule> schedule, UserContext* context, time_point entered_at) :
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

    void TimeSchedule::add_to_timeline(time_point time, UserStackEntry* entry) {
        std::lock_guard lock(modify_mutex);
        timeline.insert(std::make_pair(time, entry));
        task_notifier.notify_all();
    }

    void TimeSchedule::remove_from_timeline(time_point time, UserStackEntry* entry) {
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
}

namespace dragiyski::node_ext {
    UserContext::PreventTerminationScope::PreventTerminationScope(v8::Isolate* isolate) : _isolate(isolate) {
        auto schedule = user_code::get_time_schedule(isolate);
        if V8_LIKELY(schedule) {
            schedule->prevent_termination++;
        }
    }

    UserContext::PreventTerminationScope::~PreventTerminationScope() {
        auto schedule = user_code::get_time_schedule(_isolate);
        if V8_LIKELY(schedule) {
            if V8_UNLIKELY(--schedule->prevent_termination < 0) {
                schedule->prevent_termination = 0;
            };
        }
    }
}