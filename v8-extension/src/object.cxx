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

    v8::Local<v8::Object> object_from_property_descriptor(v8::Isolate *isolate, const v8::PropertyDescriptor &descriptor) {
        v8::HandleScope scope(isolate);

        v8::Local<v8::Name> names[5];
        v8::Local<v8::Value> values[5];
        size_t num_entries = 0;

        if (descriptor.has_configurable()) {
            names[num_entries] = StringTable::Get(isolate, "configurable");
            values[num_entries] = v8::Boolean::New(isolate, descriptor.configurable());
            ++num_entries;
        }

        if (descriptor.has_enumerable()) {
            names[num_entries] = StringTable::Get(isolate, "enumerable");
            values[num_entries] = v8::Boolean::New(isolate, descriptor.enumerable());
            ++num_entries;
        }

        if (descriptor.has_writable()) {
            names[num_entries] = StringTable::Get(isolate, "writable");
            values[num_entries] = v8::Boolean::New(isolate, descriptor.writable());
            ++num_entries;
        }

        if (descriptor.has_value()) {
            names[num_entries] = StringTable::Get(isolate, "value");
            values[num_entries] = descriptor.value();
            ++num_entries;
        }

        if (descriptor.has_get()) {
            names[num_entries] = StringTable::Get(isolate, "get");
            values[num_entries] = descriptor.get();
            ++num_entries;
        }

        if (descriptor.has_set()) {
            names[num_entries] = StringTable::Get(isolate, "set");
            values[num_entries] = descriptor.set();
            ++num_entries;
        }

        return v8::Object::New(isolate, v8::Null(isolate), names, values, num_entries);
    }

    // v8::Maybe<v8::PropertyDescriptor> property_descriptor_from_object(v8::Local<v8::Context> context, v8::Local<v8::Object> object) {
    //     static const constexpr auto __function_return_type__ = v8::Nothing<v8::PropertyDescriptor>;
    //     auto isolate = context->GetIsolate();
    //     v8::HandleScope scope(isolate);

    //     if V8_UNLIKELY(!object->IsObject()) {
    //         JS_THROW_ERROR(TypeError, context, "Property description must be an object: ", object)
    //     }

    //     JS_EXPRESSION_RETURN(value_configurable, object->Get(context, StringTable::Get(isolate, "configurable")));
    //     JS_EXPRESSION_RETURN(value_enumerable, object->Get(context, StringTable::Get(isolate, "enumerable")));
    //     JS_EXPRESSION_RETURN(value_get, object->Get(context, StringTable::Get(isolate, "get")));
    //     JS_EXPRESSION_RETURN(value_set, object->Get(context, StringTable::Get(isolate, "set")));
    //     if (JS_IS_CALLABLE(value_get) || JS_IS_CALLABLE(value_set)) {
    //         JS_EXPRESSION_RETURN(has_writable, object->Has(context, StringTable::Get(isolate, "writable")));
    //         if (has_writable) {
    //             JS_THROW_ERROR(TypeError, context, "Invalid property descriptor. Cannot both specify accessors and a value or writable attribute, ", object);
    //         }
    //         JS_EXPRESSION_RETURN(has_value, object->Has(context, StringTable::Get(isolate, "value")));
    //         if (has_value) {
    //             JS_THROW_ERROR(TypeError, context, "Invalid property descriptor. Cannot both specify accessors and a value or writable attribute, ", object);
    //         }
    //         v8::PropertyDescriptor descriptor(JS_IS_CALLABLE(value_get) ? value_get : v8::Local<v8::Value>(), JS_IS_CALLABLE(value_set) ? value_set : v8::Local<v8::Value>());
    //         descriptor.set_configurable(value_configurable->BooleanValue(isolate));
    //         descriptor.set_enumerable(value_enumerable->BooleanValue(isolate));
    //         return v8::Just(std::move(descriptor));
    //     }
    //     JS_EXPRESSION_RETURN(value_writable, object->Get(context, StringTable::Get(isolate, "writable")));
    //     JS_EXPRESSION_RETURN(value, object->Get(context, StringTable::Get(isolate, "value")));
    //     v8::PropertyDescriptor descriptor(value, value_writable->BooleanValue(isolate));
    //     descriptor.set_configurable(value_configurable->BooleanValue(isolate));
    //     descriptor.set_enumerable(value_enumerable->BooleanValue(isolate));
    //     return v8::Just(std::move(descriptor));
    // }
}
