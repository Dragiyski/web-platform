#ifndef JS_HELPER_H
#define JS_HELPER_H

#include <string>
#include <type_traits>
#include <tuple>
#include <optional>

#define JS_EXECUTE_RETURN_HANDLE(bailout, type, variable, code) \
v8::Local<type> variable; \
{ \
    v8::MaybeLocal<type> maybe = code; \
    if (maybe.IsEmpty()) { \
        return bailout; \
    } \
    variable = maybe.ToLocalChecked(); \
}

#define JS_EXECUTE_IGNORE_HANDLE(bailout, code) \
{ \
    auto maybe = code; \
    if (maybe.IsEmpty()) { \
        return bailout; \
    } \
}

#define JS_EXECUTE_RETURN(bailout, type, variable, code) \
type variable; \
{ \
    v8::Maybe<type> maybe = code; \
    if (maybe.IsNothing()) { \
        return bailout; \
    } \
    variable = maybe.ToChecked(); \
}

#define JS_EXECUTE_IGNORE(bailout, code) \
{ \
    auto maybe = code; \
    if (maybe.IsNothing()) { \
        return bailout; \
    } \
}

#define JS_TRY_CATCH_RETURN_HANDLE(bailout, isolate, type, variable, try_code, catch_code) \
v8::Local<type> variable; \
{ \
    v8::Local<v8::Value> exception; \
    { \
        v8::TryCatch tryCatch(isolate); \
        v8::MaybeLocal<type> maybe = try_code; \
        if (maybe.IsEmpty()) { \
            if (tryCatch.HasCaught()) { \
                if (!tryCatch.CanContinue()) { \
                    tryCatch.ReThrow(); \
                    return bailout; \
                } \
                exception = tryCatch.Exception(); \
            } else { \
                return; \
            } \
        } else { \
            variable = maybe.ToLocalChecked(); \
        } \
    } \
    if (!exception.IsEmpty()) catch_code; \
}


#define MAX_VALUE(a, b) (((a) > (b)) ? (a) : (b))

#define JS_COPY_ARGUMENTS(target, source, offset, length) \
v8::Local<v8::Value> target[MAX_VALUE(0, length - offset)]; \
for (int i = 0; i < length - offset; ++i) { \
    target[i] = source[offset + i]; \
}

#define JS_NOTHING(type) v8::MaybeLocal<type>()

#define VOID_NOTHING v8::Nothing<void>()

#define CPP_NOTHING(type) v8::Nothing<type>()

#define OPTIONAL_NOTHING(type) std::optional<type>();

#define NOTHING

#define JS_THROW_ERROR(bailout, context_or_isolate, ErrorType, ...) {\
    JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, message, ToErrorMessage(context_or_isolate, __VA_ARGS__));\
    v8::Local<v8::Value> error = v8::Exception::ErrorType(message);\
    get_context_or_isolate<decltype(context_or_isolate)>::isolate(context_or_isolate)->ThrowException(error);\
    return bailout;\
}

#define JS_PROPERTY_ATTRIBUTE_CONSTANT (static_cast<v8::PropertyAttribute>(v8::DontDelete | v8::ReadOnly))
#define JS_PROPERTY_ATTRIBUTE_FROZEN (static_cast<v8::PropertyAttribute>(v8::DontDelete | v8::DontEnum | v8::ReadOnly))
#define JS_PROPERTY_ATTRIBUTE_SEAL (static_cast<v8::PropertyAttribute>(v8::DontDelete | v8::DontEnum))

#define  JS_DEFINE_UINT_ATTR(bailout, context, target, srcName, srcValue, attributes) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, srcName)); \
        JS_EXECUTE_IGNORE(bailout, target->DefineOwnProperty(context, name, v8::Integer::NewFromUnsigned(context->GetIsolate(), srcValue), attributes)); \
    }

#define JS_READ_UINT_ATTR(bailout, context, target, srcName, srcValue, attributes) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, srcName)); \
        JS_EXECUTE_IGNORE(bailout, target->DefineOwnProperty(context, name, v8::Integer::NewFromUnsigned(context->GetIsolate(), srcValue), attributes)); \
    }

#define JS_DEFINE_INT_ATTR(bailout, context, target, srcName, srcValue, attributes) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, srcName)); \
        JS_EXECUTE_IGNORE(bailout, target->DefineOwnProperty(context, name, v8::Integer::New(context->GetIsolate(), srcValue), attributes)); \
    }

#define JS_DEFINE_STRING_ATTR(bailout, context, target, srcName, srcValue, attributes) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, srcName)); \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, value, ToString(context, srcValue)); \
        JS_EXECUTE_IGNORE(bailout, target->DefineOwnProperty(context, name, value, attributes)); \
    }

#define JS_DEFINE_FLOAT_ATTR(bailout, context, target, srcName, srcValue, attributes) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, srcName)); \
        JS_EXECUTE_IGNORE(bailout, target->DefineOwnProperty(context, name, v8::Number::New(context->GetIsolate(), srcValue), attributes)); \
    }

#define JS_DEFINE_INT64_ATTR(bailout, context, target, srcName, srcValue, attributes) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, srcName)); \
        JS_EXECUTE_IGNORE(bailout, target->DefineOwnProperty(context, name, v8::BigInt::New(context->GetIsolate(), srcValue), attributes)); \
    }

#define JS_DEFINE_UINT64_ATTR(bailout, context, target, srcName, srcValue, attributes) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, srcName)); \
        JS_EXECUTE_IGNORE(bailout, target->DefineOwnProperty(context, name, v8::Integer::NewFromUnsigned(context->GetIsolate(), srcValue), attributes)); \
    }

#define JS_DEFINE_UINT(bailout, context, target, srcName, srcValue) JS_DEFINE_UINT_ATTR(bailout, context, target, srcName, srcValue, v8::None)
#define JS_DEFINE_INT(bailout, context, target, srcName, srcValue) JS_DEFINE_INT_ATTR(bailout, context, target, srcName, srcValue, v8::None)
#define JS_DEFINE_STRING(bailout, context, target, srcName, srcValue) JS_DEFINE_STRING_ATTR(bailout, context, target, srcName, srcValue, v8::None)
#define JS_DEFINE_FLOAT(bailout, context, target, srcName, srcValue) JS_DEFINE_FLOAT_ATTR(bailout, context, target, srcName, srcValue, v8::None)
#define JS_DEFINE_INT64(bailout, context, target, srcName, srcValue) JS_DEFINE_INT64_ATTR(bailout, context, target, srcName, srcValue, v8::None)
#define JS_DEFINE_UINT64(bailout, context, target, srcName, srcValue) JS_DEFINE_UINT64_ATTR(bailout, context, target, srcName, srcValue, v8::None)

#define JS_READ_UINT(bailout, context, source, objectKey, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, objectKey)); \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, source->Get(context, name)); \
        if (!value->IsUint32()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "uint32_t"); \
        } \
        JS_EXECUTE_RETURN(bailout, uint32_t, primitive, value->Uint32Value(context)); \
        target = primitive; \
    }

#define JS_READ_INT(bailout, context, source, objectKey, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, objectKey)); \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, source->Get(context, name)); \
        if (!value->IsInt32()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "int32_t"); \
        } \
        JS_EXECUTE_RETURN(bailout, int32_t, primitive, value->Int32Value(context)); \
        target = primitive; \
    }

#define JS_READ_INDEX_INT(bailout, context, source, index, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, source->Get(context, index)); \
        if (!value->IsInt32()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "int32_t"); \
        } \
        JS_EXECUTE_RETURN(bailout, int32_t, primitive, value->Int32Value(context)); \
        target = primitive; \
    }

#define JS_READ_STRING(bailout, context, source, objectKey, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, objectKey)); \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, source->Get(context, name)); \
        if (!value->IsString()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "string"); \
        } \
        v8::String::Utf8Value primitive(context->GetIsolate(), value) \
        target = *primitive; \
    }

#define JS_READ_FLOAT(bailout, context, source, objectKey, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, objectKey)); \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, source->Get(context, name)); \
        if (!value->IsNumber()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "number"); \
        } \
        JS_EXECUTE_RETURN(bailout, double, primitive, value->NumberValue(context)); \
        target = static_cast<float>(primitive); \
    }

#define JS_READ_INDEX_FLOAT(bailout, context, source, index, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, source->Get(context, index)); \
        if (!value->IsNumber()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "number"); \
        } \
        JS_EXECUTE_RETURN(bailout, double, primitive, value->NumberValue(context)); \
        target = static_cast<float>(primitive); \
    }

#define JS_READ_INT64(bailout, context, source, objectKey, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, objectKey)); \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, source->Get(context, name)); \
        if (!value->IsBigInt()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "bigint"); \
        } \
        target = value.As<v8::BigInt>()->Int64Value(); \
    }

#define JS_READ_UINT64(bailout, context, objectKey, target) \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, objectKey)); \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, value, jsEvent->Get(context, name)); \
        if (!value->IsBigInt()) { \
            JS_THROW_INVALID_PROPERTY_TYPE(bailout, name.As<v8::Value>(), "bigint"); \
        } \
        target = value.As<v8::BigInt>()->Uint64Value(); \
    }

#define JS_OBJECT_GET_KEY_HANDLE(bailout, type, variable, context, object, key) \
    v8::Local<type> variable; \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, key)); \
        v8::MaybeLocal<v8::Value> maybe = object->Get(context, name); \
        if (maybe.IsEmpty()) { \
            return bailout; \
        } \
        variable = maybe.ToLocalChecked().As<type>(); \
    }

#define JS_OBJECT_HAS_KEY(bailout, variable, context, object, key) \
    bool variable; \
    { \
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, ToString(context, key)); \
        v8::Maybe<bool> maybe = object->Has(context, name); \
        if (maybe.IsNothing()) { \
            return bailout; \
        } \
        variable = maybe.ToChecked(); \
    }

namespace {
    template<typename ...T>
    struct ForEach {};

    template<typename First, typename Second, typename ... T>
    struct ForEach<First, Second, T...> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, First first, Second second, T ... rest) {
            v8::MaybeLocal<v8::String> string_first = ForEach<First>::ToString(context, first);
            if (string_first.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            v8::MaybeLocal<v8::String> string_second = ForEach<Second>::ToString(context, second);
            if (string_second.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            return ForEach<v8::Local<v8::String>, T...>::ToString(
                context,
                v8::String::Concat(context->GetIsolate(), string_first.ToLocalChecked(), string_second.ToLocalChecked()),
                rest...
            );
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, First first, Second second, T ... rest) {
            v8::MaybeLocal<v8::String> string_first = ForEach<First>::ToDetailString(context, first);
            if (string_first.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            v8::MaybeLocal<v8::String> string_second = ForEach<Second>::ToDetailString(context, second);
            if (string_second.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            return ForEach<v8::Local<v8::String>, T...>::ToDetailString(
                context,
                v8::String::Concat(context->GetIsolate(), string_first.ToLocalChecked(), string_second.ToLocalChecked()),
                rest...
            );
        }

        static v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, First first, Second second, T ... rest) {
            v8::MaybeLocal<v8::String> string_first = ForEach<First>::ToString(isolate, first);
            if (string_first.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            v8::MaybeLocal<v8::String> string_second = ForEach<Second>::ToString(isolate, second);
            if (string_second.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            return ForEach<v8::Local<v8::String>, T...>::ToString(
                isolate,
                v8::String::Concat(isolate, string_first.ToLocalChecked(), string_second.ToLocalChecked()),
                rest...
            );
        }
    };

    template<typename ... T>
    struct ForEach<v8::Local<v8::String>, v8::Local<v8::String>, T...> {
        static v8::Local<v8::String> ToString(
            v8::Local<v8::Context> context,
            v8::Local<v8::String> first,
            v8::Local<v8::String> second,
            v8::Local<T> ... rest
        ) {
            return ForEach<v8::String, T...>::ToString(context, v8::String::Concat(context->GetIsolate(), first, second), rest...);
        }

        static v8::Local<v8::String> ToDetailString(
            v8::Local<v8::Context> context,
            v8::Local<v8::String> first,
            v8::Local<v8::String> second,
            v8::Local<T> ... rest
        ) {
            return ForEach<v8::String, T...>::ToDetailString(context, v8::String::Concat(context->GetIsolate(), first, second), rest...);
        }

        static v8::Local<v8::String> ToString(
            v8::Isolate *isolate,
            v8::Local<v8::String> first,
            v8::Local<v8::String> second,
            v8::Local<T> ... rest
        ) {
            return ForEach<v8::String, T...>::ToString(isolate, v8::String::Concat(isolate, first, second), rest...);
        }

        static v8::Local<v8::String> Concat(v8::Isolate *isolate, v8::Local<v8::String> first, v8::Local<v8::String> second, v8::Local<T> ... rest) {
            return ForEach<v8::String, T...>(isolate, v8::String::Concat(isolate, first, second), rest...);
        }
    };

    template<typename T>
    struct ForEach<v8::Local<T>> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Local<T> value) {
            return value->ToString(context);
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Local<T> value) {
            return value->ToDetailString(context);
        }
    };

    template<typename T>
    struct ForEach<v8::MaybeLocal<T>> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::MaybeLocal<T> value) {
            if (value.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            return value->ToString(context);
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::MaybeLocal<T> value) {
            if (value.IsEmpty()) {
                return v8::MaybeLocal<v8::String>();
            }
            return value->ToDetailString(context);
        }
    };

    template<>
    struct ForEach<v8::Local<v8::String>> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Local<v8::String> value) {
            return value;
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Local<v8::String> value) {
            return value;
        }

        static v8::Local<v8::String> ToString(v8::Isolate *isolate, v8::Local<v8::String> value) {
            return value;
        }

        static v8::Local<v8::String> Concat(v8::Isolate *isolate, v8::Local<v8::String> first) {
            return first;
        }
    };

    template<>
    struct ForEach<v8::Local<v8::Number>> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Local<v8::Number> value) {
            return value->ToString(context);
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Local<v8::Number> value) {
            return value->ToDetailString(context);
        }
    };

    template<>
    struct ForEach<v8::MaybeLocal<v8::String>> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return value;
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::MaybeLocal<v8::String> value) {
            return value;
        }
    };

    template<typename T>
    struct ForEach<v8::Maybe<T>> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, v8::Maybe<T> value) {
            if (value.IsNothing()) {
                return JS_NOTHING(v8::String);
            }
            return ForEach<T>::ToString(context, value.FromJust());
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, v8::Maybe<T> value) {
            if (value.IsNothing()) {
                return JS_NOTHING(v8::String);
            }
            return ForEach<T>::ToDetailString(context, value.FromJust());
        }

        static v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, v8::Maybe<T> value) {
            if (value.IsNothing()) {
                return JS_NOTHING(v8::String);
            }
            return ForEach<T>::ToString(isolate, value.FromJust());
        }
    };

    template<>
    struct ForEach<const char *> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const char *value) {
            return v8::String::NewFromUtf8(context->GetIsolate(), value);
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const char *value) {
            return v8::String::NewFromUtf8(context->GetIsolate(), value);
        }

        static v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, const char *value) {
            return v8::String::NewFromUtf8(isolate, value);
        }
    };

    template<int N>
    struct ForEach<const char(&)[N]> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const char(&value)[N]) {
            std::string str(value, N);
            return v8::String::NewFromUtf8(context->GetIsolate(), str.c_str());
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const char(&value)[N]) {
            std::string str(value, N);
            return v8::String::NewFromUtf8(context->GetIsolate(), str.c_str());
        }

        static v8::Local<v8::String> ToString(v8::Isolate *isolate, const char(&value)[N]) {
            return v8::String::NewFromUtf8Literal(isolate, value);
        }
    };

    template<>
    struct ForEach<const char16_t *> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const char16_t *value) {
            return v8::String::NewFromTwoByte(context->GetIsolate(), reinterpret_cast<const uint16_t *>(value));
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const char16_t *value) {
            return v8::String::NewFromTwoByte(context->GetIsolate(), reinterpret_cast<const uint16_t *>(value));
        }

        static v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, const char16_t *value) {
            return v8::String::NewFromTwoByte(isolate, reinterpret_cast<const uint16_t *>(value));
        }
    };

    template<>
    struct ForEach<std::string> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const std::string &value) {
            return v8::String::NewFromUtf8(context->GetIsolate(), value.c_str());
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const std::string &value) {
            return v8::String::NewFromUtf8(context->GetIsolate(), value.c_str());
        }

        static v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, const std::string &value) {
            return v8::String::NewFromUtf8(isolate, value.c_str());
        }
    };

    template<>
    struct ForEach<std::u16string> {
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, const std::u16string &value) {
            return v8::String::NewFromTwoByte(context->GetIsolate(), reinterpret_cast<const uint16_t *>(value.c_str()));
        }

        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, const std::u16string &value) {
            return v8::String::NewFromTwoByte(context->GetIsolate(), reinterpret_cast<const uint16_t *>(value.c_str()));
        }

        static v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, const std::u16string &value) {
            return v8::String::NewFromTwoByte(isolate, reinterpret_cast<const uint16_t *>(value.c_str()));
        }
    };

#define FOREACH_SPECIALIZATION_LIST(V)\
        V(int)\
        V(long)\
        V(long long)\
        V(unsigned)\
        V(unsigned long)\
        V(unsigned long long)\
        V(float)\
        V(double)\
        V(long double)

#define FOREACH_STD_TO_STRING_SPECIALIZATION(Type)\
    template<>\
    struct ForEach<Type> {\
        static v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, Type value) {\
            auto string = std::to_string(value);\
            return ForEach<decltype(string)>::ToString(context, string);\
        }\
        \
        static v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, Type value) {\
            auto string = std::to_string(value);\
            return ForEach<decltype(string)>::ToDetailString(context, string);\
        }\
        \
        static v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, Type value) {\
            auto string = std::to_string(value);\
            return ForEach<decltype(string)>::ToString(isolate, string);\
        }\
    };

    FOREACH_SPECIALIZATION_LIST(FOREACH_STD_TO_STRING_SPECIALIZATION)

        struct TryCatchPassThrough {
        v8::TryCatch try_catch;

        explicit TryCatchPassThrough(v8::Isolate *isolate) : try_catch(isolate) {}

        ~TryCatchPassThrough() {
            // Required to report the exception to parent if not caught...
            try_catch.SetVerbose(true);
            try_catch.ReThrow();
        }
    };

    template<typename ... T>
    v8::MaybeLocal<v8::String> ToDetailString(v8::Local<v8::Context> context, T ... rest) {
        return ForEach<T...>::ToDetailString(context, rest...);
    }

    template<typename ... T>
    v8::MaybeLocal<v8::String> ToString(v8::Local<v8::Context> context, T ... rest) {
        return ForEach<T...>::ToString(context, rest...);
    }

    template<typename ... T>
    v8::MaybeLocal<v8::String> ToString(v8::Isolate *isolate, T ... rest) {
        return ForEach<T...>::ToString(isolate, rest...);
    }

    template<typename ... T>
    v8::MaybeLocal<v8::String> ToErrorMessage(v8::Isolate *isolate, T ... rest) {
        return ToString<T...>(isolate, rest...);
    }

    template<typename ... T>
    v8::MaybeLocal<v8::String> ToErrorMessage(v8::Local<v8::Context> context, T ... rest) {
        return ToDetailString<T...>(context, rest...);
    }

    template<typename ... T>
    v8::Local<v8::String> StringConcat(v8::Isolate *isolate, T ... rest) {
        return ForEach<T...>::Concat(isolate, rest...);
    }

    template<typename R, typename ... A>
    struct arguments_of {
        using type = std::tuple<A...>;
    };

    template<typename R, typename ... A>
    struct return_type_of {
        using type = R;
    };

    template<typename T>
    struct get_context_or_isolate {};

    template<>
    struct get_context_or_isolate<v8::Isolate *> {
        static constexpr v8::Isolate *isolate(v8::Isolate *isolate) {
            return isolate;
        }
    };

    template<>
    struct get_context_or_isolate<v8::Local<v8::Context>> {
        static v8::Isolate *isolate(v8::Local<v8::Context> &context) {
            return context->GetIsolate();
        }
    };
}

// Persistent handles are subject to change, so here we define copyable and movable handles to use:
namespace v8_handles {
    template<typename T>
    using Shared = v8::CopyablePersistentTraits<T>::CopyablePersistent;

    template<typename T>
    using Unique = v8::Global<T>;

    // For completeness
    template<typename T>
    using Local = v8::Local<T>;

    template<typename T>
    using MaybeLocal = v8::MaybeLocal<T>;

    template<typename T>
    using Maybe = v8::Maybe<T>;
};
#endif // JS_HELPER_H