#include "object.hxx"

namespace js {
    void ObjectBase::set_interface(v8::Isolate* isolate, v8::Local<v8::Object> target) {
        _interface.Reset(isolate, target);
        _interface.SetWeak(this, weak_callback, v8::WeakCallbackType::kParameter);
    }

    void ObjectBase::clear_interface(v8::Isolate* isolate) {
        _interface.Reset();
    }

    void ObjectBase::on_interface_gc(v8::Isolate* isolate) {
    }

    v8::Local<v8::Object> ObjectBase::get_interface(v8::Isolate* isolate) const {
        return _interface.Get(isolate);
    }

    void ObjectBase::weak_callback(const v8::WeakCallbackInfo<ObjectBase>& info) {
        auto isolate = info.GetIsolate();
        auto object = info.GetParameter();
        object->on_interface_gc(isolate);
        delete object;
    }

    v8::MaybeLocal<v8::String> type_of(v8::Local<v8::Context> context, v8::Local<v8::Value> value) {
        if (value->IsObject()) {
            return value.As<v8::Object>()->ObjectProtoToString(context);
        }
        return String::Create(context, "[", value->TypeOf(context->GetIsolate()), "]");
    }
}
