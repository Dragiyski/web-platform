#include "object.h"

#include <map>
#include <set>

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, std::set<ObjectWrap *>> isolate_objects;
    }

    v8::Maybe<void> ObjectWrap::initialize(v8::Isolate *isolate) {
        assert(isolate_objects.find(isolate) == isolate_objects.end());
        isolate_objects.emplace(std::piecewise_construct, std::make_tuple(isolate), std::make_tuple());
        return v8::JustVoid();
    }

    void ObjectWrap::uninitialize(v8::Isolate *isolate) {
        assert(isolate_objects.find(isolate) != isolate_objects.end());
        auto objects = isolate_objects.find(isolate)->second;
        for (auto object : objects) {
            delete object;
        }
        isolate_objects.erase(isolate);
    }

    ObjectWrap::ObjectWrap(v8::Isolate *isolate) : _isolate(isolate) {
        auto it = isolate_objects.find(isolate);
        assert(it != isolate_objects.end());
        it->second.insert(this);
    }

    ObjectWrap::~ObjectWrap() {
        auto it = isolate_objects.find(_isolate);
        assert(it != isolate_objects.end());
        it->second.erase(this);
    }
}