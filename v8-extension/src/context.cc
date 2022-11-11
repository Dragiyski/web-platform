#include "context.h"
#include "js-helper.h"
#include "function.h"

#include <mutex>
#include <map>

namespace {
    std::map<v8::Isolate*, v8::Global<v8::FunctionTemplate>> template_map;
    std::mutex template_map_mutex;
    std::map<v8::Isolate*, v8::Global<v8::Private>> context_symbol_map;
    std::mutex context_symol_map_mutex;

    void v8_throw_illegal_constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }
}

void js_global_of(const v8::FunctionCallbackInfo<v8::Value>& info) {
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

    auto signature = v8::Signature::New(isolate, tpl);
    {
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::String, name, ToString(context, "nativeFunction"));
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::FunctionTemplate, callee, v8::FunctionTemplate::New(
            isolate,
            NativeFunction,
            v8::Local<v8::Value>(),
            signature,
            1,
            v8::ConstructorBehavior::kThrow
        ));
        callee->SetClassName(name);
        tpl->PrototypeTemplate()->Set(name, callee, JS_PROPERTY_ATTRIBUTE_FROZEN);
    }
    {
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::String, name, ToString(context, "compileFunction"));
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::FunctionTemplate, callee, v8::FunctionTemplate::New(
            isolate,
            CompileFunction,
            v8::Local<v8::Value>(),
            signature,
            1,
            v8::ConstructorBehavior::kThrow
        ));
        callee->SetClassName(name);
        tpl->PrototypeTemplate()->Set(name, callee, JS_PROPERTY_ATTRIBUTE_FROZEN);
    }

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

void Context::Constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
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
    global_constructor_template->InstanceTemplate()->SetAccessCheckCallback(access_check, holder);

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

    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "global"));
        JS_EXECUTE_IGNORE(NOTHING, info.This()->DefineOwnProperty(context, name, new_context->Global(), JS_PROPERTY_ATTRIBUTE_CONSTANT));
    }

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
    std::unique_ptr<v8::MicrotaskQueue>&& task_queue
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

void Context::creation_context_after_microtasks_completed(v8::Isolate* isolate, void* ptr_wrapper) {
    auto wrapper = reinterpret_cast<Context*>(ptr_wrapper);
    if (!wrapper->auto_run_microtask_queue) {
        return;
    }
    auto context = wrapper->m_context.Get(isolate);
    context->GetMicrotaskQueue()->PerformCheckpoint(isolate);
}

bool Context::access_check(v8::Local<v8::Context> accessing_context, v8::Local<v8::Object> accessed_object, v8::Local<v8::Value> data) {
    auto isolate = accessing_context->GetIsolate();
    v8::HandleScope scope(accessing_context->GetIsolate());
    auto wrapper = node::ObjectWrap::Unwrap<Context>(data.As<v8::Object>());
    auto control_context = wrapper->m_control_context.Get(isolate);

    // While we cannot compare contexts, since each context Global() is an unique GlobalProxy (over any GlobalObject)
    // this will return true only when accessing_context and control_context are the same.
    if (control_context->Global()->SameValue(accessing_context->Global())) {
        return true;
    }
    return false;
}

// Context has:
//  - createFunction: creates a function within the contained context, calling a function within the controlling context.
// This function should pause the protection for user timing.
//  - compileFunction: creates a function by compiling user code. Return an API wrapper, the wrapper will have:
// .function: for unprotected access to the function
// .call: Reflect.call, but protected
// .apply: Reflect.apply, but protected
// .construct: Reflect.construct, but protected

void Context::NativeFunction(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    auto wrapper = node::ObjectWrap::Unwrap<Context>(info.Holder());

    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected 1 argument, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected argument[0] (options) to be an object.");
    }
    auto options = info[0].As<v8::Object>();

    v8::Local<v8::Function> callee_wrapped;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Function, value, context, options, "function");
        if (!value->IsFunction()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected required option 'function' to be a function.");
        }
        callee_wrapped = value;
    }
    
    v8::Local<v8::String> name;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::String, value, context, options, "name");
        if (!value->IsNullOrUndefined()) {
            if (!value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'name' to be a string.");
            }
            name = value;
        }
    }

    int length = 0;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "length");
        if (!js_value->IsNullOrUndefined()) {
            JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
            length = value;
        }
    }

    auto behavior = v8::ConstructorBehavior::kAllow;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "pure");
        if (!js_value->IsNullOrUndefined()) {
            if (js_value->BooleanValue(isolate)) {
                behavior = v8::ConstructorBehavior::kThrow;
            }
        }
    }
    auto callee_context = wrapper->m_context.Get(isolate);
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, callee, v8::Function::New(
        callee_context,
        native_function_callback,
        callee_wrapped,
        length,
        behavior
    ));
    if (!name.IsEmpty()) {
        callee->SetName(name);
    }

    info.GetReturnValue().Set(callee);

    // TODO: Examine v8::Isolate::AddBeforeCallEnteredCallback and v8::Isolate::AddCallCompletedCallback
    // While this function receives only isolate, we can check if the current context is a user protected context.
    // (and particularly does its global have a Private Context::Symbol holding an object, which we wrapped a Context* inside).
    // If this is the case, while we cannot retrieve the exact function the call is made to, we know it enters a user code,
    // thus appropriate timers must be executed.
}

void Context::native_function_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (!info.Data()->IsFunction()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal invocation");
    }
    v8::Local<v8::Value> args[info.Length()];
    for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
        args[i] = info[i];
    }
    auto args_array = v8::Array::New(isolate, args, info.Length());
    v8::Local<v8::Value> call_args[] = { info.This(), args_array, info.NewTarget() };
    v8::Local<v8::Value> receiver;
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Private, wrapper_symbol, Symbol(context));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, holder_value, context->Global()->GetPrivate(context, wrapper_symbol));
    if (holder_value->IsObject()) {
        receiver = holder_value;
    }
    if (receiver.IsEmpty()) {
        receiver = v8::Undefined(isolate);
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, return_value, info.Data().As<v8::Function>()->Call(context, receiver, 3, call_args));
    info.GetReturnValue().Set(return_value);
}

void Context::CompileFunction(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected 1 argument, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected argument[0] (options) to be an object.");
    }
    auto options = info[0].As<v8::Object>();
    auto wrapper = node::ObjectWrap::Unwrap<Context>(info.Holder());

    v8::Local<v8::String> name;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::String, value, context, options, "name");
        if (!value->IsNullOrUndefined()) {
            if (!value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'name' to be a string.");
            }
            name = value;
        }
    }

    std::vector<v8::Local<v8::String>> arg_names;
    arg_names.reserve(8);
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Array, js_value, context, options, "arguments");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsArray()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'arguments' to be an array.");
            }
            for (decltype(js_value->Length()) i = 0; i < js_value->Length(); ++i) {
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, arg_name_value, js_value->Get(context, i));
                if (!arg_name_value->IsString()) {
                    JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'arguments[", i, "]' to be a string.");
                }
                arg_names.push_back(arg_name_value.As<v8::String>());
            }
        }
    }

    std::vector<v8::Local<v8::Object>> scopes;
    scopes.reserve(8);
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Array, js_value, context, options, "scopes");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsArray()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'scopes' to be an array.");
            }
            for (decltype(js_value->Length()) i = 0; i < js_value->Length(); ++i) {
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, scope_value, js_value->Get(context, i));
                if (!scope_value->IsObject()) {
                    JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'scopes[", i, "]' to be an object.");
                }
                scopes.push_back(scope_value.As<v8::Object>());
            }
        }
    }


    auto source = source_from_object(context, options);
    if (!source) {
        return NOTHING;
    }
    auto callee_context = wrapper->m_context.Get(isolate);
    v8::Context::Scope callee_context_scope(callee_context);
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, callee, v8::ScriptCompiler::CompileFunction(
        callee_context,
        source.get(),
        arg_names.size(),
        arg_names.data(),
        scopes.size(),
        scopes.data(),
        v8::ScriptCompiler::kEagerCompile
    ));
    if (!name.IsEmpty()) {
        callee->SetName(name);
    }
    info.GetReturnValue().Set(callee);
}
