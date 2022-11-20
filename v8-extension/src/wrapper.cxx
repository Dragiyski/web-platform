#include "wrapper.hxx"

#include <cassert>
#include <map>
#include "js-string-table.hxx"

namespace js {
    namespace {
        std::map<v8::Isolate *, std::map<Wrapper *, std::shared_ptr<Wrapper>>> per_isolate_object_wrapper;
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
        per_isolate_object_wrapper.erase(isolate);
    }

    v8::Local<v8::Object> Wrapper::get_holder(v8::Isolate *isolate) {
        return _holder.Get(isolate);
    }

    v8::MaybeLocal<v8::Object> Wrapper::get_holder(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template) {
        using __function_return_type__ = v8::MaybeLocal<v8::Object>;

        auto holder = self->FindInstanceInPrototypeChain(class_template);
        if (!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1) {
            return holder;
        }
        auto message = StringTable::Get(isolate, "Illegal constructor");
        JS_THROW_ERROR(TypeError, isolate, message);
    }

    void Wrapper::Wrap(v8::Isolate *isolate, v8::Local<v8::Object> holder) {
        assert(!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1 && holder->GetAlignedPointerFromInternalField(0) == nullptr);
        holder->SetAlignedPointerInInternalField(0, this);
        _holder.Reset(isolate, holder);
        MakeWeak();
    }

    std::shared_ptr<Wrapper> Wrapper::find_shared_for_this(v8::Isolate *isolate, Wrapper *wrapper) {
        assert(per_isolate_object_wrapper.contains(isolate));
        assert(per_isolate_object_wrapper[isolate].contains(wrapper));
        return per_isolate_object_wrapper[isolate][wrapper];
    }

    void Wrapper::MakeWeak() {
        _holder.SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
    }

    void Wrapper::WeakCallback(const v8::WeakCallbackInfo<Wrapper> &info) {
        auto isolate = info.GetIsolate();
        assert(per_isolate_object_wrapper.contains(isolate));
        auto wrapper = info.GetParameter();
        wrapper->_holder.Reset();
        per_isolate_object_wrapper[isolate].erase(wrapper);
    }

    Wrapper::~Wrapper() {
        if (_holder.IsEmpty())
            return;
        _holder.ClearWeak();
        _holder.Reset();
    }
}
