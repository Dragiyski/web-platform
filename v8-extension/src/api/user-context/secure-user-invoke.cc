#include "time-schedule.h"

#include "../function-template.h"
#include "../../js-helper.h"
#include "../../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;

    void UserContext::secure_user_invoke(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();

        // If the protection never ran use code, using the stack is useless, as introduces a lot of work,
        // when there isn't anything waiting on the timeline.
        auto schedule = user_code::get_time_schedule(isolate);
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
        std::unique_lock<decltype(user_code::TimeSchedule::modify_mutex)> lock;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            v8::Isolate::SafeForTerminationScope safe_for_termination(isolate);
            user_code::ApiStackEntry stack_entry(schedule);
            MaybeLocal<v8::Value> return_value_maybe = callee->Call(context, receiver, 3, args);
            if (!return_value_maybe.IsEmpty()) {
                info.GetReturnValue().Set(return_value_maybe.ToLocalChecked());
                return;
            }
            if (try_catch.HasTerminated()) {
                isolate->CancelTerminateExecution();
                lock = std::unique_lock<decltype(user_code::TimeSchedule::modify_mutex)>(schedule->modify_mutex);
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
            Local<v8::String> property_name;
            {
                auto maybe = string_map::get_string(isolate, "onTerminateExecution");
                if (maybe.IsEmpty()) {
                    isolate->TerminateExecution();
                    return;
                }
                property_name = maybe.ToLocalChecked();
            }
            Local<v8::Function> termination_callback;
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
}