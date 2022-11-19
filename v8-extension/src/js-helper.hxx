#ifndef JS_HELPER_HXX
#define JS_HELPER_HXX

#include <array>
#include <charconv>
#include <cstdint>
#include <typeinfo>
#include <utility>
#include <type_traits>
#include <v8.h>

/**
 * @brief C++ does not provide a way to get the return type of the current function.
 *
 * This macro is required to use the auto return type expressions JS_EXPRESSION_*
 * Alternatively, you can define __js_current_function_return_type__ at the start
 * of the function.
 *
 * Example Usage:
 * JS_RETURN_TYPE_AWARE_FUNCTION(void, my_function(v8::Local<v8::Context> context), {
 *     // ... code goes here
 * });
 *
 * Also available for method declaration:
 * JS_RETURN_TYPE_AWARE_FUNCTION(v8::MaybeLocal<v8::Value> MyClass::my_method(v8::Local<v8::Context> context), {
 *     // ... code goes here
 * });
 *
 * Alternative without this macro:
 * v8::MaybeLocal<v8::Value> MyClass::my_method(v8::Local<v8::Context> context) {
 *     using __js_current_function_return_type__ = v8::MaybeLocal<v8::Value>;
 *     // ... code goes here
 * }
 */

#ifdef V8_HAS_ATTRIBUTE_UNUSED
#define ATTR_UNUSED __attribute__((unused))
#else
#define ATTR_UNUSED
#endif

#define CONTEXT_AWARE_FUNCTION(return_type, name, arguments, body)\
    return_type name arguments {\
        using __function_return_type__ = return_type;\
        static constexpr ATTR_UNUSED const char *__function_name__ = #name;\
        static constexpr ATTR_UNUSED const char *__function_return_type_name__ = #return_type;\
        static constexpr ATTR_UNUSED const char *__function_arguments__ = #arguments;\
        static constexpr ATTR_UNUSED const char *__function_definition__ = #return_type " " #name #arguments ";";\
        body\
    }

 /**
  * @brief Invoke single v8 expression, check for empty return and and assign it to variable if not empty.
  *
  * Invoking v8 often returns type Maybe or MaybeLocal, which will be empty if the call
  * resulted in JavaScript exception. No further calls should be made to v8, but the
  * c++ function can continue execution (usually for cleanup). If this is not necessary,
  * the function must return immediately.
  *
  * The following macro makes it possible to obtain valid result from v8 call or return
  * immediately, if the call failed.
  */
#define JS_EXPRESSION_RETURN(variable, code)\
    typename js::maybe<decltype(code)>::type variable;\
    {\
        auto _0 = code;\
        if (!js::maybe<decltype(_0)>::is_valid(_0)) {\
            return __function_return_type__();\
        }\
        variable = js::maybe<decltype(_0)>::value(_0);\
    }

  /**
   * @brief Invoke single v8 expression, check for empty return.
   *
   * Similar to JS_EXPRESSION_RETURN, but does not store the successful result.
   */
#define JS_EXPRESSION_IGNORE(code)\
    {\
        auto _0 = code;\
        if (!js::maybe<decltype(_0)>::is_valid(_0)) {\
            return __function_return_type__();\
        }\
    }

namespace js {
    template<typename T>
    using Shared = v8::CopyablePersistentTraits<T>::CopyablePersistent;

    template<typename T>
    using Unique = v8::Global<T>;

    template<typename T>
    struct maybe {};

    template<typename T>
    struct maybe<v8::Maybe<T>> {
        using type = T;
        static constexpr bool is_valid(const v8::Maybe<T> &maybe) {
            return !maybe.IsNothing();
        }

        static constexpr type value(const v8::Maybe<T> &maybe) {
            return maybe.ToChecked();
        }
    };

    template<typename T>
    struct maybe<v8::MaybeLocal<T>> {
        using type = v8::Local<T>;
        static constexpr bool is_valid(const v8::MaybeLocal<T> &maybe) {
            return !maybe.IsEmpty();
        }

        static constexpr type value(v8::MaybeLocal<T> &maybe) {
            return maybe.ToLocalChecked();
        }
    };

    template<typename T>
    struct maybe<v8::Local<T>> {
        using type = v8::Local<T>;
        static constexpr bool is_valid(const v8::Local<T> &maybe) {
            return true;
        }

        static constexpr type value(const v8::Local<T> &maybe) {
            return maybe;
        }
    };

    template<typename T>
    struct isolate_or_context_impl;

    template<>
    struct isolate_or_context_impl<v8::Local<v8::Context>> {
        static inline v8::Isolate *isolate(v8::Local<v8::Context> context) {
            return context->GetIsolate();
        }
    };

    template<>
    struct isolate_or_context_impl<v8::Isolate *> {
        static constexpr inline v8::Isolate *isolate(v8::Isolate *isolate) {
            return isolate;
        }
    };

    template<typename T>
    constexpr v8::Isolate *isolate_or_context(T value) {
        return isolate_or_context_impl<T>::isolate(value);
    }

    template<std::size_t N>
    struct string_literal {
        using type = const char(&)[N];
    };

    template<std::size_t N>
    using string_literal_t = string_literal<N>::type;

    template<typename ... T>
    struct StringList;

    template<typename T, typename = void>
    struct StringValue;

    template<typename ... T>
    struct StringConcat;

    template<typename First, typename Second, typename ... Rest>
    struct StringConcat<First, Second, Rest...> {
        static inline constexpr v8::Local<v8::String> New(v8::Isolate *isolate, First first, Second second, Rest ... rest) {
            return StringConcat<v8::Local<v8::String>, Rest...>::New(
                isolate,
                v8::String::Concat(
                    isolate,
                    StringValue<First>::New(isolate, first),
                    StringValue<Second>::New(isolate, second)
                ),
                rest...
            );
        }
    };

    template<typename T>
    struct StringConcat<T> {
        static inline constexpr v8::Local<v8::String> New(v8::Isolate *isolate, T value) {
            return StringValue<T>::New(isolate, value);
        }
    };

    template<typename First, typename Second, typename ... Rest>
    struct StringList<First, Second, Rest...> {
        static constexpr inline v8::Local<v8::String> New(v8::Isolate *isolate, const First &first, const Second &second, const Rest &...rest) {
            return StringList<v8::Local<v8::String>, v8::Local<v8::String>, Rest...>::New(
                isolate,
                StringList<const First &>::New(isolate, first),
                StringList<const Second &>::New(isolate, second),
                rest...
            );
        }

        static constexpr inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, First first, Second second, const Rest &...rest) {
            return StringList<v8::MaybeLocal<v8::String>, v8::MaybeLocal<v8::String>, Rest...>::Create(
                isolate,
                StringList<First>::Create(isolate, first),
                StringList<Second>::Create(isolate, second),
                rest...
            );
        }

        static constexpr inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, First first, Second second, const Rest &...rest) {
            return StringList<v8::MaybeLocal<v8::String>, v8::MaybeLocal<v8::String>, Rest...>::Create(
                context,
                StringList<First>::Create(context, first),
                StringList<Second>::Create(context, second),
                rest...
            );
        }

        static constexpr inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, First first, Second second, const Rest &...rest) {
            return StringList<v8::MaybeLocal<v8::String>, v8::MaybeLocal<v8::String>, Rest...>::ToString(
                context,
                StringList<First>::ToString(context, first),
                StringList<Second>::ToString(context, second),
                rest...
            );
        }

        static constexpr inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, First first, Second second, const Rest &...rest) {
            return StringList<v8::MaybeLocal<v8::String>, v8::MaybeLocal<v8::String>, Rest...>::ToDetailString(
                context,
                StringList<First>::ToDetailString(context, first),
                StringList<Second>::ToDetailString(context, second),
                rest...
            );
        }
    };


    template<typename ... T>
    struct StringList<v8::Local<v8::String>, v8::Local<v8::String>, T...> {
        static constexpr inline v8::Local<v8::String> New(v8::Isolate *isolate, v8::Local<v8::String> first, v8::Local<v8::String> second, const T &...rest) {
            return StringList<v8::Local<v8::String>, T...>::New(isolate, v8::String::Concat(isolate, first, second), rest...);
        }

        static constexpr inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, v8::Local<v8::String> first, v8::Local<v8::String> second, const T & ...rest) {
            return StringList<v8::Local<v8::String>, T...>::Create(isolate, v8::String::Concat(isolate, first, second), rest...);
        }

        static constexpr inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Local<v8::String> first, v8::Local<v8::String> second, const T & ...rest) {
            return StringList<v8::Local<v8::String>, T...>::Create(context, v8::String::Concat(context->GetIsolate(), first, second), rest...);
        }

        static constexpr inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Local<v8::String> first, v8::Local<v8::String> second, const T & ...rest) {
            return StringList<v8::Local<v8::String>, T...>::ToString(context, v8::String::Concat(context->GetIsolate(), first, second), rest...);
        }

        static constexpr inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Local<v8::String> first, v8::Local<v8::String> second, const T & ...rest) {
            return StringList<v8::Local<v8::String>, T...>::ToDetailString(context, v8::String::Concat(context->GetIsolate(), first, second), rest...);
        }
    };

    template<typename ... T>
    struct StringList<v8::MaybeLocal<v8::String>, v8::MaybeLocal<v8::String>, T...> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, v8::MaybeLocal<v8::String> first, v8::MaybeLocal<v8::String> second, const T & ...rest) {
            if (!maybe<decltype(first)>::is_valid(first) || !maybe<decltype(second)>::is_valid(second)) {
                return {};
            }
            return StringList<v8::MaybeLocal<v8::String>, T...>::Create(isolate, v8::String::Concat(isolate, maybe<decltype(first)>::value(first), maybe<decltype(second)>::value(second)), rest...);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> first, v8::MaybeLocal<v8::String> second, const T & ...rest) {
            if (!maybe<decltype(first)>::is_valid(first) || !maybe<decltype(second)>::is_valid(second)) {
                return {};
            }
            return StringList<v8::MaybeLocal<v8::String>, T...>::Create(context, v8::String::Concat(context->GetIsolate(), maybe<decltype(first)>::value(first), maybe<decltype(second)>::value(second)), rest...);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> first, v8::MaybeLocal<v8::String> second, const T & ...rest) {
            if (!maybe<decltype(first)>::is_valid(first) || !maybe<decltype(second)>::is_valid(second)) {
                return {};
            }
            return StringList<v8::MaybeLocal<v8::String>, T...>::ToString(context, v8::String::Concat(context->GetIsolate(), maybe<decltype(first)>::value(first), maybe<decltype(second)>::value(second)), rest...);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> first, v8::MaybeLocal<v8::String> second, const T & ...rest) {
            if (!maybe<decltype(first)>::is_valid(first) || !maybe<decltype(second)>::is_valid(second)) {
                return {};
            }
            return StringList<v8::MaybeLocal<v8::String>, T...>::ToDetailString(context, v8::String::Concat(context->GetIsolate(), maybe<decltype(first)>::value(first), maybe<decltype(second)>::value(second)), rest...);
        }
    };

    template<typename T>
    struct StringList<T> {
        static inline v8::Local<v8::String> New(v8::Isolate *isolate, const T &value) {
            return StringValue<T>::New(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const T &value) {
            return StringValue<decltype(value)>::Create(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const T &value) {
            return StringValue<decltype(value)>::Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const T &value) {
            return StringValue<decltype(value)>::ToString(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const T &value) {
            return StringValue<decltype(value)>::ToDetailString(context, value);
        }
    };

    struct String {
        template<typename ... T>
        static constexpr inline v8::Local<v8::String> New(v8::Isolate *isolate, const T& ... args) {
            return StringList<decltype(args)...>::New(isolate, args...);
        }

        template<typename ... T>
        static constexpr inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const T& ... args) {
            return StringList<decltype(args)...>::Create(isolate, args...);
        }

        template<typename ... T>
        static constexpr inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const T& ... args) {
            return StringList<decltype(args)...>::Create(context, args...);
        }

        template<typename ... T>
        static constexpr inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const T& ... args) {
            return StringList<decltype(args)...>::ToString(context, args...);
        }

        template<typename ... T>
        static constexpr inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const T& ... args) {
            return StringList<decltype(args)...>::ToDetailString(context, args...);
        }
    };

    template<>
    struct StringValue<v8::Local<v8::String>> {
        static inline v8::Local<v8::String> New(v8::Isolate *isolate, const v8::Local<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const v8::Local<v8::String> &value) {
            return New(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const v8::Local<v8::String> &value) {
            return New(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::Local<v8::String> &value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::Local<v8::String> &value) {
            return Create(context, value);
        }
    };

    template<std::size_t length>
    struct StringValue<const char(&)[length]> {
        static inline v8::Local<v8::String> New(v8::Isolate *isolate, const char(&literal)[length]) {
            return v8::String::NewFromUtf8Literal(isolate, literal);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const char(&literal)[length]) {
            return New(isolate, literal);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const char(&literal)[length]) {
            return New(context->GetIsolate(), literal);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const char(&literal)[length]) {
            return Create(context, literal);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const char(&literal)[length]) {
            return Create(context, literal);
        }
    };

    template<>
    struct StringValue<const char *const &> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const char *const &value) {
            return v8::String::NewFromUtf8(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const char *const &value) {
            return v8::String::NewFromUtf8(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const char *const &value) {
            return v8::String::NewFromUtf8(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const char *const &value) {
            return v8::String::NewFromUtf8(context->GetIsolate(), value);
        }
    };

    template<>
    struct StringValue<v8::MaybeLocal<v8::String>> {
        v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, v8::MaybeLocal<v8::String> value) {
            return value;
        }

        v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return value;
        }

        v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return Create(context, value);
        }

        v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return Create(context, value);
        }
    };

    template<>
    struct StringValue<std::string> {
        v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const std::string &string) {
            return v8::String::NewFromUtf8(isolate, string.c_str(), v8::NewStringType::kNormal, string.size());
        }

        v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const std::string &string) {
            return Create(context->GetIsolate(), string);
        }

        v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const std::string &string) {
            return Create(context, string);
        }

        v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const std::string &string) {
            return Create(context, string);
        }
    };

    template<>
    struct StringValue<bool> {
        v8::Local<v8::String> New(v8::Isolate *isolate, bool value) {
            if (value) {
                return v8::String::NewFromUtf8Literal(isolate, "true");
            } else {
                return v8::String::NewFromUtf8Literal(isolate, "false");
            }
        }

        v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, bool value) {
            return New(isolate, value);
        }

        v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, bool value) {
            return New(context->GetIsolate(), value);
        }

        v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, bool value) {
            return Create(context, value);
        }

        v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, bool value) {
            return Create(context, value);
        }
    };

    template<typename T>
    struct StringValue<T, std::enable_if_t<std::is_integral_v<T>>> {
        v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, T value) {
            // 64 is sufficient for base-2 for the largest integral type
            std::array<char, 64> string;
            if (auto [end, err] = std::to_chars(string.data(), string.data() + string.size(), value, 10); err == std::errc()) {
                return v8::String::NewFromUtf8(isolate, string.data(), v8::NewStringType::kNormal, end - string.data());
            } else {
                return String::Create(isolate, "[", typeid(T).name, ": ", std::make_error_code(err).message(), "]");
            }
        }

        v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, T value) {
            return Create(context->GetIsolate(), value);
        }

        v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, T value) {
            return Create(context, value);
        }

        v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, T value) {
            return Create(context, value);
        }
    };

    template<typename T>
    struct StringValue<T, std::enable_if_t<std::is_floating_point_v<T>>> {
        v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, T value) {
            // 64 is sufficient for base-2 for the largest integral type
            std::array<char, 1024> string;
            if (auto [end, err] = std::to_chars(string.data(), string.data() + string.size(), value, std::chars_format::fixed); err == std::errc()) {
                return v8::String::NewFromUtf8(isolate, string.data(), v8::NewStringType::kNormal, end - string.data());
            } else {
                return String::Create(isolate, "[", typeid(T).name, ": ", std::make_error_code(err).message(), "]");
            }
        }

        v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, T value) {
            return Create(context->GetIsolate(), value);
        }

        v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, T value) {
            return Create(context, value);
        }

        v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, T value) {
            return Create(context, value);
        }
    };

    template<typename T>
    struct StringValue<const v8::Local<T> &, std::enable_if_t<std::is_base_of_v<v8::Primitive, T>>> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const v8::Local<T> &value) {
            return value->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::Local<T> &value) {
            return value->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::Local<T> &value) {
            return value->ToDetailString(context);
        }
    };

    template<typename T>
    struct StringValue<const v8::Local<T> &, std::enable_if_t<std::is_base_of_v<v8::Value, T>>> {
        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::Local<T> &value) {
            return value->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::Local<T> &value) {
            return value->ToDetailString(context);
        }
    };

    template<typename T>
    struct StringValue<const v8::MaybeLocal<T> &, std::enable_if_t<std::is_base_of_v<v8::Primitive, T>>> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const v8::MaybeLocal<T> &value) {
            if (value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::MaybeLocal<T> &value) {
            if (value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::MaybeLocal<T> &value) {
            if (value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToDetailString(context);
        }
    };

    template<typename T>
    struct StringValue<const v8::MaybeLocal<T> &, std::enable_if_t<std::is_base_of_v<v8::Value, T>>> {
        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::MaybeLocal<T> &value) {
            if (value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::MaybeLocal<T> &value) {
            if (value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToDetailString(context);
        }
    };

    template<>
    struct StringValue<const v8::Local<v8::String> &> {
        static inline v8::Local<v8::String> New(v8::Isolate *isolate, const v8::Local<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const v8::Local<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const v8::Local<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::Local<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::Local<v8::String> &value) {
            return value;
        }
    };

    template<>
    struct StringValue<const v8::MaybeLocal<v8::String> &> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate *isolate, const v8::MaybeLocal<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const v8::MaybeLocal<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::MaybeLocal<v8::String> &value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::MaybeLocal<v8::String> &value) {
            return value;
        }
    };

    template<typename T, typename ... S>
    struct error_message_impl;

    template<typename ... S>
    struct error_message_impl<v8::Local<v8::Context>, S...> {
        static inline v8::MaybeLocal<v8::String> create(v8::Local<v8::Context> context, const S &... string_list) {
            return String::ToDetailString(context, string_list...);
        }
    };

    template<typename ... S>
    struct error_message_impl<v8::Isolate *, S...> {
        static inline v8::MaybeLocal<v8::String> create(v8::Isolate *isolate, const S &... string_list) {
            return String::Create(isolate, string_list...);
        }
    };

    template<typename T, typename ... S>
    v8::MaybeLocal<v8::String> ErrorMessage(T isolate_or_context_value, const S &... string_list) {
        return error_message_impl<T, S...>::create(isolate_or_context_value, string_list...);
    }

#define JS_THROW_ERROR(Type, isolate_or_context_value, ...)\
        {\
            JS_EXPRESSION_RETURN(_0, ::js::ErrorMessage((isolate_or_context_value), __VA_ARGS__));\
            auto _1 = v8::Exception::Type(_0);\
            ::js::isolate_or_context(isolate_or_context_value)->ThrowException(_1);\
            return __function_return_type__();\
        }

#define JS_PROPERTY_ATTRIBUTE_CONST (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::ReadOnly))
#define JS_PROPERTY_ATTRIBUTE_STATIC (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontEnum))
#define JS_PROPERTY_ATTRIBUTE_SEAL (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum))
#define JS_PROPERTY_ATTRIBUTE_VOLATIE (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum | v8::PropertyAttribute::ReadOnly))
#define JS_PROPERTY_ATTRIBUTE_DYNAMIC (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum))
#define JS_PROPERTY_ATTRIBUTE_DEFAULT (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::None))
}

#endif /* JS_HELPER_HXX */