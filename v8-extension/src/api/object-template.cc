#include "object-template.h"

#include <cassert>
#include <map>
#include "../js-helper.h"

#include "function-template.h"

namespace dragiyski::node_ext {
    DECLARE_API_WRAPPER_BODY(ObjectTemplate);

    v8::Maybe<void> ObjectTemplate::initialize_template(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> class_template) {
        class_template->Inherit(Template::get_template(isolate));
        return v8::JustVoid();
    }

    void ObjectTemplate::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto holder = info.This()->FindInstanceInPrototypeChain(get_template(isolate));
        if (
            !info.IsConstructCall() ||
            holder.IsEmpty() ||
            !holder->IsObject() ||
            holder.As<v8::Object>()->InternalFieldCount() < 1
            ) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal constructor");
        }

        v8::Local<v8::FunctionTemplate> constructor;
        if (info.Length() >= 1) {
            if (!info[0]->IsNullOrUndefined()) {
                if (!info[0]->IsObject()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
                }
                JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, info[0].As<v8::Object>()));
                constructor = wrapper->value(isolate);
            }
        }

        auto value = v8::ObjectTemplate::New(isolate, constructor);

        auto wrapper = new ObjectTemplate(isolate, value);
        wrapper->Wrap(holder);

        info.GetReturnValue().Set(info.This());
    }

    ObjectTemplate::ObjectTemplate(v8::Isolate *isolate, v8::Local<v8::ObjectTemplate> value) : _value(isolate, value) {}

    v8::Local<v8::ObjectTemplate> ObjectTemplate::value(v8::Isolate *isolate) {
        return _value.Get(isolate);
    }

    v8::MaybeLocal<v8::Object> ObjectTemplate::New(v8::Local<v8::Context> context, v8::Local<v8::ObjectTemplate> value) {
        auto isolate = context->GetIsolate();
        v8::EscapableHandleScope scope(isolate);

        auto class_template = get_template(isolate);
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Object, holder, class_template->InstanceTemplate()->NewInstance(context));
        assert(holder->InternalFieldCount() >= 1);

        auto wrapper = new ObjectTemplate(isolate, value);
        wrapper->Wrap(holder);

        return scope.Escape(holder);
    }
}
