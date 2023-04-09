#ifndef JS_HELPER_HXX
#define JS_HELPER_HXX

#include <array>
#include <charconv>
#include <cstdint>
#include <typeinfo>
#include <utility>
#include <system_error>
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
        auto __js_expression_return_maybe__ = code;\
        if (!js::maybe<decltype(__js_expression_return_maybe__)>::is_valid(__js_expression_return_maybe__)) {\
            return __function_return_type__();\
        }\
        variable = js::maybe<decltype(__js_expression_return_maybe__)>::value(__js_expression_return_maybe__);\
    }

  /**
   * @brief Invoke single v8 expression, check for empty return.
   *
   * Similar to JS_EXPRESSION_RETURN, but does not store the successful result.
   */
#define JS_EXPRESSION_IGNORE(code)\
    {\
        auto __js_expression_ignore_maybe__ = code;\
        if (!js::maybe<decltype(__js_expression_ignore_maybe__)>::is_valid(__js_expression_ignore_maybe__)) {\
            return __function_return_type__();\
        }\
    }

namespace js {
    template<typename T>
    using Shared = typename v8::CopyablePersistentTraits<T>::CopyablePersistent;

    template<typename T>
    using Unique = v8::Global<T>;

    template<typename T>
    struct maybe {
        using type = T;
        using maybe_type = v8::Maybe<T>;

        static constexpr auto return_type = v8::Nothing<T>;

        static constexpr bool is_valid(const T& maybe) {
            return true;
        }

        static constexpr type value(const T& maybe) {
            return maybe;
        }

        static constexpr maybe_type to_maybe(T && value) {
            v8::Just(value);
        }
    };

    template<>
    struct maybe<void> {
        using type = void;
        using maybe_type = v8::Maybe<void>;

        static auto return_type() {
            return v8::JustVoid();
        }
    };

    template<typename T>
    struct maybe<v8::Maybe<T>> {
        using type = T;
        using maybe_type = v8::Maybe<T>;
        static constexpr auto return_value = v8::Nothing<T>;

        static constexpr bool is_valid(const v8::Maybe<T>& maybe) {
            return !maybe.IsNothing();
        }

        static constexpr type value(const v8::Maybe<T>& maybe) {
            return maybe.ToChecked();
        }
    };

    template<typename T>
    struct maybe<v8::MaybeLocal<T>> {
        using type = v8::Local<T>;
        using maybe_type = v8::MaybeLocal<T>;

        static constexpr auto return_value() {
            return maybe_type();
        }

        static constexpr bool is_valid(const v8::MaybeLocal<T> maybe) {
            return !maybe.IsEmpty();
        }

        static constexpr type value(v8::MaybeLocal<T>& maybe) {
            return maybe.ToLocalChecked();
        }
    };

    template<typename T>
    struct maybe<v8::Local<T>> {
        using type = v8::Local<T>;
        static constexpr bool is_valid(const v8::Local<T>& maybe) {
            return true;
        }

        static constexpr type value(const v8::Local<T>& maybe) {
            return maybe;
        }
    };

    template<typename T>
    struct isolate_or_context_impl;

    template<>
    struct isolate_or_context_impl<v8::Local<v8::Context>> {
        static inline v8::Isolate* isolate(v8::Local<v8::Context> context) {
            return context->GetIsolate();
        }
    };

    template<>
    struct isolate_or_context_impl<v8::Isolate*> {
        static constexpr inline v8::Isolate* isolate(v8::Isolate* isolate) {
            return isolate;
        }
    };

    template<typename T>
    constexpr v8::Isolate* isolate_or_context(T value) {
        return isolate_or_context_impl<T>::isolate(value);
    }

    template<std::size_t N>
    using string_literal_t = const char(&)[N];

    template<typename T, typename = void>
    struct string_type {
        using type = T; // Everything else is kept as is.
    };

    template<typename T>
    struct string_type<const v8::Local<T>&> {
        using type = v8::Local<T>; // References to v8::Local are copied.
    };

    template<typename T>
    struct string_type<const v8::MaybeLocal<T>&> {
        using type = v8::MaybeLocal<T>; // References to v8::MaybeLocal are copied.
    };

    template<std::size_t N>
    struct string_type<string_literal_t<N>> {
        using type = string_literal_t<N>; // String literal references are kept as is.
    };

    template<typename T>
    struct string_type<const T* const&> {
        using type = const T*;
    };

    template<typename T>
    struct string_type<T* const&> {
        using type = T*;
    };

    template<typename T>
    struct string_type<const T&, std::enable_if_t<std::is_integral_v<T>>> {
        using type = T; // References to scalars are removed.
    };

    template<typename T>
    struct string_type<const T&, std::enable_if_t<std::is_floating_point_v<T>>> {
        using type = T; // References to scalars are removed.
    };

    template<typename T>
    using string_type_t = typename string_type<T>::type;

    template<typename T, typename = void>
    struct string_value_impl;

    template<std::size_t N>
    struct string_value_impl<string_literal_t<N>> {
        static inline v8::Local<v8::String> New(v8::Isolate* isolate, string_literal_t<N> value) {
            return v8::String::NewFromUtf8Literal(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, string_literal_t<N> value) {
            return New(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, string_literal_t<N> value) {
            return New(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, string_literal_t<N> value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, string_literal_t<N> value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, string_literal_t<N> value) {
            return New(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, string_literal_t<N> value) {
            return Create(context, isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, string_literal_t<N> value) {
            return Create(context, isolate, value);
        }
    };

    template<>
    struct string_value_impl<v8::Local<v8::String>> {
        static inline v8::Local<v8::String> New(v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }
    };

    template<>
    struct string_value_impl<v8::MaybeLocal<v8::String>> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, v8::MaybeLocal<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }
    };

    template<>
    struct string_value_impl<const char*> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, const char* value) {
            return v8::String::NewFromUtf8(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const char* value) {
            return Create(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const char* value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const char* value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, const char* value) {
            return Create(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, const char* value) {
            return Create(context, isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, const char* value) {
            return Create(context, isolate, value);
        }
    };

    template<>
    struct string_value_impl<const std::string&> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, const std::string& value) {
            return v8::String::NewFromUtf8(isolate, value.c_str());
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const std::string& value) {
            return Create(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const std::string& value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const std::string& value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, const std::string& value) {
            return Create(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, const std::string& value) {
            return Create(context, isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, const std::string& value) {
            return Create(context, isolate, value);
        }
    };

    template<>
    struct string_value_impl<bool> {
        static inline v8::Local<v8::String> New(v8::Isolate* isolate, bool value) {
            if (value) {
                return v8::String::NewFromUtf8Literal(isolate, "true");
            } else {
                return v8::String::NewFromUtf8Literal(isolate, "false");
            }
        }

        v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, bool value) {
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

        v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, bool value) {
            return New(isolate, value);
        }

        v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, bool value) {
            return Create(context, isolate, value);
        }

        v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, bool value) {
            return Create(context, isolate, value);
        }
    };

    struct String {
        template<typename ... T>
        static constexpr v8::Local<v8::String> New(v8::Isolate* isolate, const T& ... args);

        template<typename ... T>
        static constexpr v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, const T& ... args);

        template<typename ... T>
        static constexpr v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const T& ... args);

        template<typename ... T>
        static constexpr v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const T& ... args);

        template<typename ... T>
        static constexpr v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const T& ... args);
    };

    template<typename T>
    struct string_value_impl<T, std::enable_if_t<std::is_integral_v<T>>> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, T value) {
            // 64 is sufficient for base-2 for the largest integral type
            std::array<char, 64> string;
            if (auto [end, err] = std::to_chars(string.data(), string.data() + string.size(), value, 10); err == std::errc()) {
                return v8::String::NewFromUtf8(isolate, string.data(), v8::NewStringType::kNormal, end - string.data());
            } else {
                return String::Create(isolate, "[", typeid(T).name(), ": ", std::make_error_code(err).message(), "]");
            }
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const T& value) {
            return Create(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const T& value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const T& value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, const T& value) {
            return Create(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, const T& value) {
            return Create(context, isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, const T& value) {
            return Create(context, isolate, value);
        }
    };

    template<typename T>
    struct string_value_impl<T, std::enable_if_t<std::is_floating_point_v<T>>> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, T value) {
            // 64 is sufficient for base-2 for the largest integral type
            std::array<char, 1024> string;
            if (auto [end, err] = std::to_chars(string.data(), string.data() + string.size(), value, std::chars_format::fixed); err == std::errc()) {
                return v8::String::NewFromUtf8(isolate, string.data(), v8::NewStringType::kNormal, end - string.data());
            } else {
                return String::Create(isolate, "[", typeid(T).name(), ": ", std::make_error_code(err).message(), "]");
            }
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, const T& value) {
            return Create(context->GetIsolate(), value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const T& value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const T& value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, const T& value) {
            return Create(isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, const T& value) {
            return Create(context, isolate, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, const T& value) {
            return Create(context, isolate, value);
        }
    };

    template<typename T>
    struct string_value_impl<v8::MaybeLocal<T>, std::enable_if_t<std::is_base_of_v<v8::Primitive, T>>> {
        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::MaybeLocal<T> value) {
            if V8_UNLIKELY(value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::MaybeLocal<T> value) {
            if V8_UNLIKELY(value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::MaybeLocal<T> value) {
            if V8_UNLIKELY(value.IsEmpty()) {
                return {};
            }
            return value.ToLocalChecked()->ToDetailString(context);
        }

        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::MaybeLocal<T> value) {
            return Create(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::MaybeLocal<T> value) {
            return ToString(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, v8::MaybeLocal<T> value) {
            return ToDetailString(context, value);
        }
    };

    template<typename T>
    struct string_value_impl<v8::Local<T>, std::enable_if_t<std::is_base_of_v<v8::Value, T>>> {
        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const v8::Local<T>& value) {
            return value->ToString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const v8::Local<T>& value) {
            return value->ToDetailString(context);
        }

        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, const v8::Local<T>& value) {
            return ToString(context, value);
        }

        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, const v8::Local<T>& value) {
            return ToDetailString(context, value);
        }
    };

    template<typename ... T>
    struct string_concat_impl;

    template<typename ... Rest>
    struct string_concat_impl<v8::Local<v8::String>, v8::Local<v8::String>, Rest...> {
        static inline v8::Local<v8::String> concat(v8::Isolate* isolate, v8::Local<v8::String> first, v8::Local<v8::String> second, Rest ... rest) {
            return string_concat_impl<v8::Local<v8::String>, Rest...>::concat(isolate, v8::String::Concat(isolate, first, second), rest...);
        }
    };

    template<typename ... Rest>
    struct string_concat_impl<v8::MaybeLocal<v8::String>, v8::MaybeLocal<v8::String>, Rest...> {
        static inline v8::MaybeLocal<v8::String> concat(v8::Isolate* isolate, v8::MaybeLocal<v8::String> first, v8::MaybeLocal<v8::String> second, Rest ... rest) {
            if V8_UNLIKELY(first.IsEmpty() || second.IsEmpty()) {
                return {};
            }
            return string_concat_impl<v8::Local<v8::String>, Rest...>::concat(isolate, v8::String::Concat(isolate, first.ToLocalChecked(), second.ToLocalChecked()), rest...);
        }
    };

    template<typename ... Rest>
    struct string_concat_impl<v8::Local<v8::String>, v8::MaybeLocal<v8::String>, Rest...> {
        static inline v8::MaybeLocal<v8::String> concat(v8::Isolate* isolate, v8::Local<v8::String> first, v8::MaybeLocal<v8::String> second, Rest ... rest) {
            if V8_UNLIKELY(second.IsEmpty()) {
                return {};
            }
            return string_concat_impl<v8::Local<v8::String>, Rest...>::concat(isolate, v8::String::Concat(isolate, first, second.ToLocalChecked()), rest...);
        }
    };

    template<>
    struct string_concat_impl<v8::Local<v8::String>> {
        static inline v8::Local<v8::String> concat(v8::Isolate* isolate, v8::Local<v8::String> value) {
            return value;
        }
    };

    template<>
    struct string_concat_impl<v8::MaybeLocal<v8::String>> {
        static inline v8::MaybeLocal<v8::String> concat(v8::Isolate* isolate, v8::MaybeLocal<v8::String> value) {
            return value;
        }
    };

    template<typename ... T>
    struct string_impl {
        static inline v8::Local<v8::String> New(v8::Isolate* isolate, T... values) {
            return string_concat_impl<decltype(string_value_impl<T>::New(isolate, values))...>::concat(isolate, string_value_impl<T>::New(isolate, values)...);
        };
        static inline v8::MaybeLocal<v8::String> Create(v8::Isolate* isolate, T... values) {
            return string_concat_impl<decltype(string_value_impl<T>::Create(isolate, values))...>::concat(isolate, string_value_impl<T>::Create(isolate, values)...);
        };
        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, T... values) {
            auto isolate = context->GetIsolate();
            return string_impl<T...>::Create(context, isolate, values...);
        };
        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, T... values) {
            auto isolate = context->GetIsolate();
            return string_impl<T...>::ToString(context, isolate, values...);
        };
        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, T... values) {
            auto isolate = context->GetIsolate();
            return string_impl<T...>::ToDetailString(context, isolate, values...);
        };
        static inline v8::MaybeLocal<v8::String> Create(v8::Local<v8::Context> context, v8::Isolate* isolate, T... values) {
            return string_concat_impl<decltype(string_value_impl<T>::Create(context, isolate, values))...>::concat(isolate, string_value_impl<T>::Create(context, isolate, values)...);
        };
        static inline v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Isolate* isolate, T... values) {
            return string_concat_impl<decltype(string_value_impl<T>::ToString(context, isolate, values))...>::concat(isolate, string_value_impl<T>::ToString(context, isolate, values)...);
        };
        static inline v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Isolate* isolate, T... values) {
            return string_concat_impl<decltype(string_value_impl<T>::ToDetailString(context, isolate, values))...>::concat(isolate, string_value_impl<T>::ToDetailString(context, isolate, values)...);
        };
    };

    template<typename ... T>
    constexpr v8::Local<v8::String> String::New(v8::Isolate* isolate, const T& ... args) {
        return string_impl<string_type_t<const T&>...>::New(isolate, static_cast<string_type_t<const T&>>(args)...);
    }

    template<typename ... T>
    constexpr v8::MaybeLocal<v8::String> String::Create(v8::Isolate* isolate, const T& ... args) {
        return string_impl<string_type_t<const T&>...>::Create(isolate, static_cast<string_type_t<const T&>>(args)...);
    }

    template<typename ... T>
    constexpr v8::MaybeLocal<v8::String> String::Create(v8::Local<v8::Context> context, const T& ... args) {
        return string_impl<string_type_t<const T&>...>::Create(context, static_cast<string_type_t<const T&>>(args)...);
    }

    template<typename ... T>
    constexpr v8::MaybeLocal<v8::String> String::ToString(v8::Local<v8::Context> context, const T& ... args) {
        return string_impl<string_type_t<const T&>...>::ToString(context, static_cast<string_type_t<const T&>>(args)...);
    }

    template<typename ... T>
    constexpr v8::MaybeLocal<v8::String> String::ToDetailString(v8::Local<v8::Context> context, const T& ... args) {
        return string_impl<string_type_t<const T&>...>::ToDetailString(context, static_cast<string_type_t<const T&>>(args)...);
    }

    template<typename T, typename ... S>
    struct error_message_impl;

    template<typename ... S>
    struct error_message_impl<v8::Local<v8::Context>, S...> {
        static inline v8::MaybeLocal<v8::String> create(v8::Local<v8::Context> context, const S &... string_list) {
            return String::ToDetailString(context, string_list...);
        }
    };

    template<typename ... S>
    struct error_message_impl<v8::Isolate*, S...> {
        static inline v8::MaybeLocal<v8::String> create(v8::Isolate* isolate, const S &... string_list) {
            return String::Create(isolate, string_list...);
        }
    };

    template<typename T, typename ... S>
    v8::MaybeLocal<v8::String> ErrorMessage(T isolate_or_context_value, const S &... string_list) {
        return error_message_impl<T, S...>::create(isolate_or_context_value, string_list...);
    }

#define JS_THROW_ERROR(Type, isolate_or_context_value, ...)\
        {\
            JS_EXPRESSION_RETURN(__js_throw_error_message__, ::js::ErrorMessage((isolate_or_context_value), __VA_ARGS__));\
            auto __js_throw_error_exception__ = v8::Exception::Type(__js_throw_error_message__);\
            ::js::isolate_or_context(isolate_or_context_value)->ThrowException(__js_throw_error_exception__);\
            return __function_return_type__();\
        }

#define JS_PROPERTY_ATTRIBUTE_CONST (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::ReadOnly))
#define JS_PROPERTY_ATTRIBUTE_STATIC (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontEnum))
#define JS_PROPERTY_ATTRIBUTE_SEAL (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontDelete | v8::PropertyAttribute::DontEnum))
#define JS_PROPERTY_ATTRIBUTE_VOLATIE (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum | v8::PropertyAttribute::ReadOnly))
#define JS_PROPERTY_ATTRIBUTE_DYNAMIC (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum))
#define JS_PROPERTY_ATTRIBUTE_DEFAULT (static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::None))

    struct Source : public v8::ScriptCompiler::Source {
        template<typename ... Args>
        Source(Args &&... args) : v8::ScriptCompiler::Source(std::forward<Args>(args)...) {}
    };
}

#endif /* JS_HELPER_HXX */