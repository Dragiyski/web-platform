#include "context.h"

#include <cassert>
#include <map>
#include <set>
#include <list>
#include "../js-helper.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <ranges>
#include <variant>

#include "../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    DECLARE_API_WRAPPER_BODY(Context);

    Maybe<void> Context::initialize_template(v8::Isolate* isolate, Local<v8::FunctionTemplate> class_template) {
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "for");
            auto value = v8::FunctionTemplate::New(
                isolate,
                for_object,
                {},
                {},
                1,
                v8::ConstructorBehavior::kThrow
            );
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        return v8::JustVoid();
    }

    void Context::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal constructor");
    }

    MaybeLocal<v8::Context> Context::GetCreationContext(Local<v8::Context> context, Local<v8::Object> object) {
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        v8::Local<v8::Context> creation_context;
        {
            v8::TryCatch try_catch(isolate);
            try_catch.SetVerbose(false);
            try_catch.SetCaptureMessage(false);
            auto maybe_context = object->GetCreationContext();
            if (!maybe_context.IsEmpty()) {
                creation_context = maybe_context.ToLocalChecked();
                goto has_context;
            }
            if (!try_catch.HasCaught()) {
                goto empty_context;
            }
            try_catch.ReThrow();
            return JS_NOTHING(v8::Context);
        }
    empty_context:
        JS_THROW_ERROR(JS_NOTHING(v8::Context), isolate, ReferenceError, "No creation context found");
    has_context:
        assert(!creation_context.IsEmpty());
        return creation_context;
    }

    MaybeLocal<v8::Object> Context::ForObject(Local<v8::Context> context, Local<v8::Object> object) {
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Context, creation_context, GetCreationContext(context, object));
        auto global = creation_context->Global();
        auto key = symbol_this(isolate);
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Value, holder_value, global->GetPrivate(context, key));
        if (holder_value->IsObject()) {
            return holder_value.As<v8::Object>();
        }

        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Object), v8::Object, holder, get_template(isolate)->InstanceTemplate()->NewInstance(context));
        JS_EXECUTE_IGNORE(JS_NOTHING(v8::Object), global->SetPrivate(context, key, holder));
        auto wrapper = new Context(isolate, creation_context);
        wrapper->Wrap(holder, holder);

        return holder;
    }

    void Context::for_object(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "arguments[0]: not an object");
        }

        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, holder, ForObject(context, info[0].As<v8::Object>()));

        JS_EXECUTE_RETURN(NOTHING, Context*, wrapper, Context::unwrap(isolate, holder));
        auto self = wrapper->container(isolate);
        info.GetReturnValue().Set(self);
    }
}
