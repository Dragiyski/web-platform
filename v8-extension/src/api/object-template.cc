#include "object-template.h"

#include <cassert>
#include <map>
#include "../js-helper.h"

#include "function-template.h"
#include "../string-table.h"

namespace dragiyski::node_ext {
    DECLARE_API_WRAPPER_BODY(ObjectTemplate);

    v8::Maybe<void> ObjectTemplate::initialize_template(v8::Isolate* isolate, Local<v8::FunctionTemplate> class_template) {
        class_template->Inherit(Template::get_template(isolate));
        auto signature = v8::Signature::New(isolate, class_template);
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "create");
            auto value = v8::FunctionTemplate::New(isolate, create, {}, signature, 0, v8::ConstructorBehavior::kThrow);
            value->SetClassName(name);
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
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

        Local<v8::FunctionTemplate> constructor;
        if (info.Length() >= 1) {
            if (!info[0]->IsNullOrUndefined()) {
                if (!info[0]->IsObject()) {
                    JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
                }
                JS_EXECUTE_RETURN(NOTHING, FunctionTemplate*, wrapper, FunctionTemplate::unwrap(isolate, info[0].As<v8::Object>()));
                constructor = wrapper->this_function_template(isolate);
            }
        }

        auto value = v8::ObjectTemplate::New(isolate, constructor);

        auto wrapper = new ObjectTemplate(isolate, value);
        wrapper->Wrap(holder, info.This());

        info.GetReturnValue().Set(info.This());
    }

    ObjectTemplate::ObjectTemplate(v8::Isolate* isolate, Local<v8::ObjectTemplate> value) :
        Template(isolate),
        _value(isolate, value) {}

    Local<v8::Template> ObjectTemplate::this_template(v8::Isolate* isolate) {
        return _value.Get(isolate);
    }

    Local<v8::ObjectTemplate> ObjectTemplate::this_object_template(v8::Isolate* isolate) {
        return _value.Get(isolate);
    }

    MaybeLocal<v8::Object> ObjectTemplate::New(Local<v8::Context> context, Local<v8::ObjectTemplate> value) {
        auto isolate = context->GetIsolate();
        v8::EscapableHandleScope scope(isolate);

        auto class_template = get_template(isolate);
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Object, holder, class_template->InstanceTemplate()->NewInstance(context));
        assert(holder->InternalFieldCount() >= 1);

        auto wrapper = new ObjectTemplate(isolate, value);
        wrapper->Wrap(holder, holder);

        return scope.Escape(wrapper->container(isolate));
    }

    void ObjectTemplate::create(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.This()->FindInstanceInPrototypeChain(get_template(isolate));
        if (
            holder.IsEmpty() ||
            !holder->IsObject() ||
            holder.As<v8::Object>()->InternalFieldCount() < 1
            ) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal invocation");
        }

        JS_EXECUTE_RETURN(NOTHING, ObjectTemplate*, wrapper, ObjectTemplate::unwrap(isolate, holder));
        auto object_template = wrapper->this_object_template(isolate);

        // TODO: Handle arguments[0] as potential wrapper of v8::Context
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, result, object_template->NewInstance(context));
        info.GetReturnValue().Set(result);
    }
}
