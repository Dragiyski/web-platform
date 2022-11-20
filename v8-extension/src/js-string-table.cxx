#include "js-string-table.hxx"

#include <map>
#include "js-helper.hxx"

namespace js {
    using string_map_t = std::map<const char *, Shared<v8::String>>;
    namespace {
        std::map<v8::Isolate *, string_map_t> per_isolate_string_map;
    }

    void StringTable::initialize(v8::Isolate *isolate) {
        assert(per_isolate_string_map.find(isolate) == per_isolate_string_map.end());
        per_isolate_string_map.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple()
        );
    }

    void StringTable::uninitialize(v8::Isolate *isolate) {
        per_isolate_string_map.erase(isolate);
    }

    v8::Local<v8::String> StringTable::find_in_map(v8::Isolate *isolate, const char *string) {
        auto map_node = per_isolate_string_map.find(isolate);
        assert(map_node != per_isolate_string_map.end());
        auto string_map = map_node->second;
        auto string_node = string_map.find(string);
        if (string_node == string_map.end()) {
            return {};
        }
        return string_node->second.Get(isolate);
    }

    void StringTable::insert_in_map(v8::Isolate *isolate, const char *c_str, v8::Local<v8::String> js_str) {
        per_isolate_string_map[isolate][c_str].Reset(isolate, js_str);
    }
}
