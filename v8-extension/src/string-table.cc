#include "string-table.h"

#include <cassert>
#include <map>

#include "js-helper.h"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate*, std::map<const char*, v8::Global<v8::String>>> per_isolate_string_map;
    }

    void string_map::initialize(v8::Isolate *isolate) {
        assert(per_isolate_string_map.find(isolate) == per_isolate_string_map.end());
        per_isolate_string_map.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple());
    }

    void string_map::uninitialize(v8::Isolate *isolate) {
        assert(per_isolate_string_map.find(isolate) != per_isolate_string_map.end());
        per_isolate_string_map.erase(isolate);
    }

    v8::MaybeLocal<v8::String> get_string(v8::Isolate* isolate, const char* string, std::size_t length) {
        auto it = per_isolate_string_map.find(isolate);
        if(it == per_isolate_string_map.end()) {
            return JS_NOTHING(v8::String);
        }
        auto string_map = it->second;
        {
            auto it = string_map.find(string);
            if (it != string_map.find(string)) {
                return it->second.Get(isolate);
            }
        }
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::String), v8::String, js_string, v8::String::NewFromUtf8(
            isolate,
            string,
            v8::NewStringType::kNormal,
            length
        ));
        string_map.emplace(std::piecewise_construct, std::forward_as_tuple(string), std::forward_as_tuple(isolate, js_string));
        return js_string;
    }
}
