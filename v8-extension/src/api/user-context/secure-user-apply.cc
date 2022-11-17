#include "time-schedule.h"

#include "../function-template.h"
#include "../../js-helper.h"
#include "../../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;

    void UserContext::secure_user_apply(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        auto schedule = user_code::get_time_schedule(isolate);
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

        std::unique_lock<decltype(user_code::TimeSchedule::modify_mutex)> lock;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            v8::Isolate::SafeForTerminationScope safe_for_termination(isolate);
            user_code::UserStackEntry stack_entry(schedule, wrapper);
            auto maybe_return_value = callee->Call(wrapper->value(isolate), info[1], args->Length(), args_list);
            if (!maybe_return_value.IsEmpty()) {
                info.GetReturnValue().Set(maybe_return_value.ToLocalChecked());
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