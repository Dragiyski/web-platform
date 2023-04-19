#ifndef JS_ERROR_MESSAGE_HXX
#define JS_ERROR_MESSAGE_HXX

#include <v8.h>
#include <functional>
#include <cassert>
#include "js-helper.hxx"
#include "js-string-table.hxx"

#define JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(variable, code, context, ...)\
    typename js::maybe<decltype(code)>::type variable;\
    {\
        auto __js_expression_return_with_error_prefix_maybe__ = try_catch_prefix_message<js::maybe<decltype(code)>::maybe_type>((context), [&]()->js::maybe<decltype(code)>::maybe_type {\
            return code;\
        }, __VA_ARGS__);\
        if V8_UNLIKELY(!js::maybe<decltype(__js_expression_return_with_error_prefix_maybe__)>::is_valid(__js_expression_return_with_error_prefix_maybe__)) {\
            return __function_return_type__();\
        }\
        variable = js::maybe<decltype(__js_expression_return_with_error_prefix_maybe__)>::value(__js_expression_return_with_error_prefix_maybe__);\
    }

#define JS_EXPRESSION_IGNORE_WITH_ERROR_PREFIX(code, context, ...)\
    {\
        auto __js_expression_return_maybe__ = try_catch_prefix_message<js::maybe<decltype(code)>::maybe_type>((context), [&]()->js::maybe<decltype(code)>::maybe_type {\
            return code;\
        }, __VA_ARGS__);\
        if V8_UNLIKELY(!js::maybe<decltype(__js_expression_return_maybe__)>::is_valid(__js_expression_return_maybe__)) {\
            return __function_return_type__();\
        }\
    }

namespace js {
    template<typename T, typename Func, typename ... Args>
    typename maybe<T>::maybe_type try_catch_prefix_message(v8::Local<v8::Context> context, Func callee, Args ... args) {
        static constexpr auto __function_return_type__ = maybe<T>::return_value;
        auto isolate = context->GetIsolate();
        v8::TryCatch try_catch(isolate);
        auto maybe_value = callee();
        if V8_LIKELY(maybe<T>::is_valid(maybe_value)) {
            return maybe_value;
        }
        assert(try_catch.HasCaught());
        if V8_LIKELY(try_catch.CanContinue() && !try_catch.Exception().IsEmpty() && try_catch.Exception()->IsNativeError()) {
            auto exception = try_catch.Exception().As<v8::Object>();
            auto message_property = StringTable::Get(isolate, "message");
            JS_EXPRESSION_RETURN(old_message_value, exception->Get(context, message_property));
            if V8_LIKELY(old_message_value->IsString()) {
                auto old_message = old_message_value.As<v8::String>();
                v8::Local<v8::String> new_message;
                if V8_LIKELY(old_message->Length() > 0) {
                    JS_EXPRESSION_RETURN(new_message_value, String::Create(context, args..., ": ", old_message));
                    new_message = new_message_value;
                } else {
                    JS_EXPRESSION_RETURN(new_message_value, String::Create(context, args...));
                    new_message = new_message_value;
                }
                JS_EXPRESSION_IGNORE(exception->Set(context, message_property, new_message));
            }
        }
        try_catch.ReThrow();
        return __function_return_type__();
    }
}

#endif /* JS_ERROR_MESSAGE_HXX */