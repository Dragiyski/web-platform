#ifndef JS_STRING_TABLE_HXX
#define JS_STRING_TABLE_HXX

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <v8.h>

namespace js {
    namespace {
        template<std::size_t N>
        struct string_table_literal {
            char value[N];

            constexpr string_table_literal(const char (&literal)[N]) {
                std::copy_n(literal, N, value);
            }
        };
    }

    struct StringTable {
        /**
         * String table uses the new C++20 ability to have string literals as template argument.
         * Literals lifetime is eternal: they are added to constant memory loaded with the program and
         * stay loaded until the program terminates.
         * 
         * The kInternalized prevents creation of multiple JavaScript strings if the same string is already in memory.
         * This is undesired "slow" operation, as a hash of strings is kept, so that calling with the same string won't result in new object.
         * We only use this for literals, as it will be called only once per literal.
         * 
         * Problems:
         * The old design was using string literal argument as:
         * template<std::size_t N> Get(const char (&literal)[N])
         * 
         * Unfotunately c++20 cannot pass argument compile-time literal to template argument... (although they are both compile-time)
         * But we can pass a type as template argument, when that type contains literal array. This allows the method to be called as:
         * 
         * StringTable::Get<"Hellow, wolrd!">(isolate);
         * 
         * The static local variable will be initialized once no matter how many times the method is called.
         * There will be one template initialization per unique string literal.
         * This will allow calling this method multiple times, but have only one NewFromUtf8Literal.
         * 
         * Note: It is very very likely this method to be inlined.
         */
        template<string_table_literal literal>
        static inline v8::Local<v8::String> Get(v8::Isolate *isolate) {
            static auto js_str = v8::String::NewFromUtf8Literal(isolate, literal.value, v8::NewStringType::kInternalized);
            return js_str;
        }
    };
}

#endif /* JS_STRING_TABLE_HXX */
