#include "time-schedule.h"

#include "../function-template.h"
#include "../../js-helper.h"
#include "../../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;

    namespace {
        // TODO: Find better method to call Reflect.construct with new.target, perhaps report bug to v8
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

    void UserContext::secure_user_construct(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        auto schedule = user_code::get_time_schedule(isolate);
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
        Local<v8::Function> newTarget;
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

        std::unique_lock<decltype(user_code::TimeSchedule::modify_mutex)> lock;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            v8::Isolate::SafeForTerminationScope safe_for_termination(isolate);
            user_code::UserStackEntry stack_entry(schedule, wrapper);
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
