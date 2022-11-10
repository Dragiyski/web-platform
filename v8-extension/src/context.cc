#include "context.h"
#include "js-helper.h"

#include <mutex>
#include <map>

namespace {
    std::map<v8::Isolate *, v8::Global<v8::FunctionTemplate>> template_map;
    std::mutex template_map_mutex;
    std::map<v8::Isolate *, v8::Global<v8::Private>> context_symbol_map;
    std::mutex context_symol_map_mutex;

    void v8_throw_illegal_constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }
}

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    info.GetReturnValue().SetNull();
    if (info.Length() >= 1 && info[0]->IsObject()) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, creation_context, info[0].As<v8::Object>()->GetCreationContext());
        info.GetReturnValue().Set(creation_context->Global());
    }
}

v8::MaybeLocal<v8::FunctionTemplate> Context::Template(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    v8::EscapableHandleScope scope(isolate);
    std::lock_guard template_map_lock(template_map_mutex);
    auto location = template_map.find(isolate);
    if (location != template_map.end()) {
        auto result = location->second.Get(isolate);
        return scope.Escape(result);
    }
    auto tpl = v8::FunctionTemplate::New(isolate, Constructor, v8::Local<v8::Value>(), v8::Local<v8::Signature>());
    JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::String, name, ToString(context, "Context"));
    tpl->SetClassName(name);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    template_map.insert(std::make_pair(isolate, v8::Global<v8::FunctionTemplate>(isolate, tpl)));
    return scope.Escape(tpl);
}

v8::MaybeLocal<v8::Private> Context::Symbol(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    v8::EscapableHandleScope scope(isolate);
    std::lock_guard symbol_lock(context_symol_map_mutex);
    auto location = context_symbol_map.find(isolate);
    if (location != context_symbol_map.end()) {
        auto result = location->second.Get(isolate);
        return scope.Escape(result);
    }
    JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Private), v8::String, name, ToString(context, "Context"));
    auto result = v8::Private::New(isolate, name);
    context_symbol_map.insert(std::make_pair(isolate, v8::Global<v8::Private>(isolate, result)));
    return scope.Escape(result);
}

void Context::Constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (!info.IsConstructCall()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }

    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::FunctionTemplate, context_template, Template(context));

    auto holder = info.This()->FindInstanceInPrototypeChain(context_template);
    if (holder.IsEmpty() || holder->InternalFieldCount() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }

    v8::Local<v8::Object> options;
    if (info.Length() < 1) {
        options = v8::Object::New(isolate);
    } else if (!info[0]->IsNullOrUndefined()) {
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, if specified");
        }
        options = info[0].As<v8::Object>();
    }

    v8::Local<v8::String> context_name;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::String, js_value, context, options, "name");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'name' to be a string");
            }
            context_name = js_value;
        }
    }

    auto global_constructor_template = v8::FunctionTemplate::New(isolate, v8_throw_illegal_constructor, holder);
    if (!context_name.IsEmpty()) {
        global_constructor_template->SetClassName(context_name);
    }

    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Private, context_symbol, Symbol(context));

    auto task_queue = v8::MicrotaskQueue::New(isolate, v8::MicrotasksPolicy::kExplicit);

    auto new_context = v8::Context::New(
        isolate,
        nullptr,
        global_constructor_template->InstanceTemplate(),
        v8::MaybeLocal<v8::Value>(),
        v8::DeserializeInternalFieldsCallback(),
        task_queue.get()
    );

    new_context->AllowCodeGenerationFromStrings(true);
    new_context->Global()->SetPrivate(context, context_symbol, holder);
    auto wrapper = new Context(context, new_context, std::move(task_queue));
    wrapper->Wrap(holder);

    context->GetMicrotaskQueue()->AddMicrotasksCompletedCallback(creation_context_after_microtasks_completed);

    info.GetReturnValue().Set(info.This());
}

Context::Context(
    v8::Local<v8::Context> control_context,
    v8::Local<v8::Context> wrap_context,
    std::unique_ptr<v8::MicrotaskQueue> &&task_queue
) : m_isolate(control_context->GetIsolate()),
m_control_context(control_context->GetIsolate(), control_context),
m_context(control_context->GetIsolate(), wrap_context),
m_task_queue(std::move(task_queue)) {}

Context::~Context() {
    if (!m_isolate->IsDead()) {
        auto control_context = m_control_context.Get(m_isolate);
        auto microtask_queue = control_context->GetMicrotaskQueue();
        if (microtask_queue != nullptr) {
            microtask_queue->RemoveMicrotasksCompletedCallback(creation_context_after_microtasks_completed);
        }
    }
}

void Context::creation_context_after_microtasks_completed(v8::Isolate *isolate, void *ptr_wrapper) {
    auto wrapper = reinterpret_cast<Context *>(ptr_wrapper);
    if (!wrapper->auto_run_microtask_queue) {
        return;
    }
    auto context = wrapper->m_context.Get(isolate);
    context->GetMicrotaskQueue()->PerformCheckpoint(isolate);
}

void Context::NativeFunction(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    // Create a function that calls v8 API function, which should declare to context it is not running any user code.
}
