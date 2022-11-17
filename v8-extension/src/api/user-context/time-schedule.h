#include "../user-context.h"

#include <map>
#include <set>
#include <list>
#include <chrono>
#include <variant>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace dragiyski::node_ext::user_code {
    using namespace v8_handles;

    struct APITimeEntry;
    struct UserTimeEntry;
    using ScheduleEntry = std::variant<APITimeEntry, UserTimeEntry>;
    using time_point = std::chrono::steady_clock::time_point;
    using duration = std::chrono::steady_clock::duration;
    struct TimeSchedule;

    std::shared_ptr<TimeSchedule> get_time_schedule(v8::Isolate* isolate);

    struct StackEntry {
        std::shared_ptr<TimeSchedule> schedule;
        StackEntry* prev, * next;
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
        UserContext* context;
        std::optional<duration> max_entry_duration, max_user_duration;
        std::optional<time_point> max_entry_timepoint, max_user_timepoint;

        virtual void on_stack_enter();
        virtual void on_stack_leave(duration api_duration, duration user_duration);
        virtual inline bool is_user_entry() { return true; }

        UserStackEntry(std::shared_ptr<TimeSchedule> schedule, UserContext* context, time_point entered_at = std::chrono::steady_clock::now());
        virtual ~UserStackEntry() override;
    };

    struct TimeSchedule : std::enable_shared_from_this<TimeSchedule> {
        v8::Isolate* isolate;
        StackEntry* root, * top;
        std::recursive_mutex modify_mutex;
        std::mutex task_mutex;
        std::condition_variable task_notifier;
        std::thread time_thread;
        int prevent_termination = 0;
        bool has_terminated = false;

        std::multimap<time_point, UserStackEntry*> timeline;

        static void thread_function(std::shared_ptr<TimeSchedule> schedule);

        void add_to_timeline(time_point, UserStackEntry*);
        void remove_from_timeline(time_point, UserStackEntry*);

        explicit TimeSchedule(v8::Isolate* isolate);

        ~TimeSchedule();
    };
}
