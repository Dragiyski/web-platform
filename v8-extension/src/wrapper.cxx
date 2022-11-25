#include "wrapper.hxx"

#include <cassert>
#include <map>
#include <set>
#include "js-string-table.hxx"

namespace js {
    namespace {
        std::map<v8::Isolate *, std::set<Wrapper *>> per_isolate_object_wrapper;
    }

    void Wrapper::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_object_wrapper.contains(isolate));
        per_isolate_object_wrapper.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple()
        );
    }

    void Wrapper::uninitialize(v8::Isolate *isolate) {
        for (auto *wrapper : per_isolate_object_wrapper[isolate]) {
            delete wrapper;
        }
        per_isolate_object_wrapper.erase(isolate);
    }

    void dispose(v8::Isolate *isolate, Wrapper *wrapper) {
        assert(per_isolate_object_wrapper.contains(isolate));
        v8::HandleScope scope(isolate);
        auto holder = wrapper->get_holder(isolate);
        if (!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1) {
            holder->SetAlignedPointerInInternalField(0, nullptr);
        }
        per_isolate_object_wrapper[isolate].erase(wrapper);
        delete wrapper;
    }

    v8::Local<v8::Object> Wrapper::get_holder(v8::Isolate *isolate) {
        return _holder.Get(isolate);
    }

    v8::MaybeLocal<v8::Object> Wrapper::get_holder(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template, const char *type) {
        using __function_return_type__ = v8::MaybeLocal<v8::Object>;

        auto holder = self->FindInstanceInPrototypeChain(class_template);
        if (!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1) {
            return holder;
        }
        JS_THROW_ERROR(TypeError, isolate, "Failed to convert value to '", type, "'.");
    }

    void Wrapper::Wrap(v8::Isolate *isolate, v8::Local<v8::Object> holder) {
        assert(!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1);
        assert(per_isolate_object_wrapper.contains(isolate));
        assert(!per_isolate_object_wrapper[isolate].contains(this));
        per_isolate_object_wrapper[isolate].insert(this);
        holder->SetAlignedPointerInInternalField(0, this);
        _holder.Reset(isolate, holder);
        MakeWeak();
    }

    void Wrapper::MakeWeak() {
        _holder.SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
    }

    void Wrapper::WeakCallback(const v8::WeakCallbackInfo<Wrapper> &info) {
        auto isolate = info.GetIsolate();
        assert(per_isolate_object_wrapper.contains(isolate));
        auto wrapper = info.GetParameter();
        // TODO: Maybe dispose will work here. It is unclear if temporary local handle is okay.
        per_isolate_object_wrapper[isolate].erase(wrapper);
        delete wrapper;
    }

    Wrapper::~Wrapper() {
        if (_holder.IsEmpty())
            return;
        _holder.ClearWeak();
        _holder.Reset();
    }
}
