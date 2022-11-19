#include "wrapper.hxx"

#include <cassert>
#include <map>
#include "js-string-table.hxx"

namespace js {
    namespace {
        std::map<v8::Isolate*, std::map<Wrapper *, std::shared_ptr<Wrapper>>> per_isolate_object_wrapper;
    }

    void Wrapper::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_object_wrapper.contains(isolate));
        per_isolate_object_wrapper.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple()
        );
    }

    void Wrapper::uninitialize(v8::Isolate* isolate) {
        assert(per_isolate_object_wrapper.contains(isolate));
        per_isolate_object_wrapper.erase(isolate);
    }

    v8::Local<v8::Object> Wrapper::self(v8::Isolate* isolate) const {
        return _self.Get(isolate);
    }

    v8::Local<v8::Object> Wrapper::holder(v8::Isolate* isolate) {
        return persistent().Get(isolate);
    }

    v8::MaybeLocal<v8::Object> Wrapper::Wrap(v8::Isolate* isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template) {
        using __function_return_type__ = v8::MaybeLocal<v8::Object>;

        auto holder = self->FindInstanceInPrototypeChain(class_template);
        if (holder.IsEmpty() || !holder->IsObject() || holder->InternalFieldCount() < 1) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }

        node::ObjectWrap::Wrap(holder);
        _self.Reset(isolate, self);

        return holder;
    }

    std::shared_ptr<Wrapper> Wrapper::find_shared_for_this(v8::Isolate* isolate, Wrapper *wrapper) {
        assert(per_isolate_object_wrapper.contains(isolate));
        assert(per_isolate_object_wrapper[isolate].contains(wrapper));
        return per_isolate_object_wrapper[isolate][wrapper];        
    }
}
