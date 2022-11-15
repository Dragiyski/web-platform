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

#include "../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    DECLARE_API_WRAPPER_BODY_MORE(UserContext, initialize_more, uninitialize_more);


    namespace {
        struct APITimeEntry;
        struct UserTimeEntry;
        using ScheduleEntry = std::variant<APITimeEntry, UserTimeEntry>;

        struct TimeEntry {
            std::chrono::steady_clock::time_point entered_at;

            static constexpr TimeEntry& from(ScheduleEntry& entry);
        };

        struct APITimeEntry : public TimeEntry {};

        struct UserTimeEntry : public TimeEntry {
            UserContext* context;

            std::chrono::steady_clock::duration max_entry_time;
            std::chrono::steady_clock::duration max_user_time;

            std::chrono::steady_clock::duration user_time;
        };

        struct TimeSchedule {
            v8::Isolate* isolate;
            std::list<ScheduleEntry> entry_stack;
            std::multimap<UserContext*, decltype(entry_stack)::iterator> per_context_entry;
            std::multimap<std::chrono::steady_clock::time_point, decltype(entry_stack)::iterator> timeline;

            std::recursive_mutex modify_mutex;
            std::mutex task_mutex;
            std::condition_variable task_notifier;
            bool is_active;

            static inline void thread_function(std::shared_ptr<TimeSchedule> schedule) {
                while (schedule->is_active) {
                    std::optional<std::chrono::steady_clock::time_point> task_time;
                    bool should_interrupt = false;
                    {
                        std::lock_guard lock(schedule->modify_mutex);
                        auto now = std::chrono::steady_clock::now();
                        auto next_task = schedule->timeline.upper_bound(now);
                        if (next_task != schedule->timeline.end()) {
                            bool need_user_time = false;
                            if (next_task == schedule->timeline.begin()) {
                                task_time = next_task->first;
                                goto process_task;
                            }
                            auto task = next_task;
                            do {
                                --task;
                                auto time_entry_variant = *task->second;
                                auto time_entry = TimeEntry::from(time_entry_variant);
                            } while (task != schedule->timeline.begin());
                        }
                    }
                process_task:
                    {
                        std::unique_lock lock(schedule->task_mutex);
                    }
                }
            }
        };
    }
}
