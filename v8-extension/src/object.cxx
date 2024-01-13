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

    v8::MaybeLocal<v8::Value> object_or_function_call(v8::Local<v8::Context> context, v8::Local<v8::Value> callee, v8::Local<v8::Value> receiver, int argc, v8::Local<v8::Value> argv[]) {
        using __function_return_type__ = v8::MaybeLocal<v8::Value>;
        auto isolate = context->GetIsolate();
        if (callee->IsFunction()) {
            return callee.As<v8::Function>()->Call(context, receiver, argc, argv);
        } else if (callee->IsObject() && callee.As<v8::Object>()->IsCallable()) {
            return callee.As<v8::Object>()->CallAsFunction(context, receiver, argc, argv);
        }
        JS_THROW_ERROR(TypeError, isolate, "The callee is not a function");
    }
}
