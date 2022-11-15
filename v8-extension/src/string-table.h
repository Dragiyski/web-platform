#ifndef V8EXT_STRING_TABLE_H
#define V8EXT_STRING_TABLE_H

#include <v8.h>
#include "js-helper.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    namespace string_map {
        void initialize(v8::Isolate *isolate);
        void uninitialize(v8::Isolate *isolate);

        MaybeLocal<v8::String> get_string(v8::Isolate *isolate, const char *string, std::size_t length);

        template<std::size_t N>
        MaybeLocal<v8::String> get_string(v8::Isolate *isolate, const char(&string)[N]) {
            return get_string(isolate, static_cast<const char *>(string), N - 1);
        }
    }
}

#define JS_PROPERTY_NAME(bailout, name, isolate, literal) JS_EXECUTE_RETURN_HANDLE(bailout, v8::String, name, dragiyski::node_ext::string_map::get_string(isolate, literal))

#define JS_OBJECT_GET_LITERAL_KEY(bailout, variable, context, object, property)\
    Local<v8::Value> variable;\
    {\
        auto isolate = (context)->GetIsolate();\
        JS_PROPERTY_NAME(bailout, _0, isolate, property);\
        JS_EXECUTE_RETURN_HANDLE(bailout, v8::Value, _1, (object)->Get(context, _0));\
        variable = _1;\
    }

#define JS_OBJECT_SET_LITERAL_KEY(bailout, variable, context, object, property, value)\
    {\
        auto isolate = (context)->GetIsolate();\
        JS_PROPERTY_NAME(bailout, _0, isolate, property);\
        JS_EXECUTE_IGNORE(bailout, (object)->Set(context, _0, value));\
    }

#endif /* V8EXT_STRING_TABLE_H */