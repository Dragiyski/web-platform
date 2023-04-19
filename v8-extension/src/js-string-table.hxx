#ifndef JS_STRING_TABLE_HXX
#define JS_STRING_TABLE_HXX

#include <cassert>
#include <cstdint>
#include <v8.h>

namespace js {
    struct StringTable {
        static void initialize(v8::Isolate *isolate);
        static void uninitialize(v8::Isolate *isolate);

        template<std::size_t N>
        static inline v8::Local<v8::String> Get(v8::Isolate *isolate, const char (&literal)[N]) {
            auto c_str = static_cast<const char *>(literal);
            auto js_str = find_in_map(isolate, c_str);
            if (js_str.IsEmpty()) {
                js_str = v8::String::NewFromUtf8Literal(isolate, literal, v8::NewStringType::kInternalized);
                assert(!js_str.IsEmpty());
                insert_in_map(isolate, c_str, js_str);
            }
            return js_str;
        }

    private:
        static v8::Local<v8::String> find_in_map(v8::Isolate *isolate, const char *string);
        static void insert_in_map(v8::Isolate *isolate, const char *c_str, v8::Local<v8::String> js_str);
    };
}

#endif /* JS_STRING_TABLE_HXX */
