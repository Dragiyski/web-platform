#include "object.h"

#include <map>
#include <set>

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, std::set<ObjectWrap *>> per_isolate_objects;
        std::map<ObjectWrap *, v8::Isolate *> creation_isolate;
    }

    v8::Maybe<void> ObjectWrap::initialize(v8::Isolate *isolate) {
        assert(per_isolate_objects.find(isolate) == per_isolate_objects.end());
        per_isolate_objects.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple());
        return v8::JustVoid();
    }

    void ObjectWrap::uninitialize(v8::Isolate *isolate) {
        assert(per_isolate_objects.find(isolate) != per_isolate_objects.end());
        auto objects = per_isolate_objects.find(isolate)->second;
        for (auto object : objects) {
            delete object;
        }
        per_isolate_objects.erase(isolate);
    }

    ObjectWrap::ObjectWrap(v8::Isolate *isolate) {
        per_isolate_objects.find(isolate)->second.insert(this);
        creation_isolate.insert(std::make_pair(this, isolate));
    }

    ObjectWrap::~ObjectWrap() {
        auto isolate = creation_isolate.find(this)->second;
        auto object_set = per_isolate_objects.find(isolate)->second;
        object_set.erase(this);
        v8::HandleScope scope(isolate);
        // If the JS object is alive, but the wrapping object is deleted,
        // this ensure the situation can be recognized.
        persistent().Get(isolate)->SetAlignedPointerInInternalField(0, nullptr);
    }

    v8::Local<v8::Object> ObjectWrap::container(v8::Isolate *isolate) const {
        return _container.Get(isolate);
    }

    // Sometimes the holder and the container does match. For example in JS class X extends ObjectWrap,
    // would create valid new, where holder != this, holder has the internal fields, this is what is presented to the user.
    void ObjectWrap::Wrap(v8::Local<v8::Object> holder, v8::Local<v8::Object> container) {
        node::ObjectWrap::Wrap(holder);
        _container.Reset(v8::Isolate::GetCurrent(), container);
    }
}