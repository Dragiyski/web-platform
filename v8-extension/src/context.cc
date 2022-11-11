#include "context.h"
#include "js-helper.h"
#include "function.h"

#include <mutex>
#include <map>
#include <set>
#include <node.h>

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    info.GetReturnValue().SetNull();
    if (info.Length() >= 1 && info[0]->IsObject()) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, creation_context, info[0].As<v8::Object>()->GetCreationContext());
        info.GetReturnValue().Set(creation_context->Global());
    }
}

std::map<v8::Isolate *, Context::per_isolate_value> Context::per_isolate;

v8::Maybe<void> Context::Init(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    per_isolate.emplace(std::piecewise_construct, std::make_tuple(isolate), std::make_tuple());
    JS_EXECUTE_IGNORE(VOID_NOTHING, init_template(context));
    JS_EXECUTE_IGNORE(VOID_NOTHING, init_symbol(context));

    auto node_env = node::GetCurrentEnvironment(context);
    if (node_env != nullptr) {
        node::AtExit(node_env, reinterpret_cast<void(*)(void *)>(at_exit), isolate);
        auto node_platform = node::GetMultiIsolatePlatform(node_env);
        node_platform->AddIsolateFinishedCallback(isolate, reinterpret_cast<void(*)(void *)>(on_isolate_dispose), isolate);
    }
    return v8::JustVoid();
}

v8::Maybe<void> Context::init_template(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    auto function_template = v8::FunctionTemplate::New(isolate, Constructor, v8::Local<v8::Value>(), v8::Local<v8::Signature>());
    JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, name, ToString(context, "Context"));
    function_template->SetClassName(name);
    function_template->InstanceTemplate()->SetInternalFieldCount(1);

    auto signature = v8::Signature::New(isolate, function_template);
    {
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, name, ToString(context, "nativeFunction"));
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::FunctionTemplate, callee, v8::FunctionTemplate::New(
            isolate,
            NativeFunction,
            v8::Local<v8::Value>(),
            signature,
            1,
            v8::ConstructorBehavior::kThrow
        ));
        callee->SetClassName(name);
        function_template->PrototypeTemplate()->Set(name, callee, JS_PROPERTY_ATTRIBUTE_FROZEN);
    }
    {
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, name, ToString(context, "compileFunction"));
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::FunctionTemplate, callee, v8::FunctionTemplate::New(
            isolate,
            CompileFunction,
            v8::Local<v8::Value>(),
            signature,
            1,
            v8::ConstructorBehavior::kThrow
        ));
        callee->SetClassName(name);
        function_template->PrototypeTemplate()->Set(name, callee, JS_PROPERTY_ATTRIBUTE_FROZEN);
    }
    {
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, name, ToString(context, "dispose"));
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::FunctionTemplate, callee, v8::FunctionTemplate::New(
            isolate,
            Dispose,
            v8::Local<v8::Value>(),
            signature,
            1,
            v8::ConstructorBehavior::kThrow
        ));
        callee->SetClassName(name);
        function_template->PrototypeTemplate()->Set(name, callee, JS_PROPERTY_ATTRIBUTE_FROZEN);
    }
    {
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, name, ToString(context, "global"));
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::FunctionTemplate, callee, v8::FunctionTemplate::New(
            isolate,
            Dispose,
            v8::Local<v8::Value>(),
            signature,
            1,
            v8::ConstructorBehavior::kThrow
        ));
        callee->SetClassName(name);
        function_template->InstanceTemplate()->SetAccessor(
            name,
            GetGlobal,
            nullptr,
            v8::Local<v8::Value>(),
            v8::AccessControl::ALL_CAN_READ,
            JS_PROPERTY_ATTRIBUTE_FROZEN,
            v8::SideEffectType::kHasNoSideEffect
        );
    }

    auto it = per_isolate.find(isolate);
    it->second.class_template.Reset(isolate, function_template);

    return v8::JustVoid();
}

v8::Maybe<void> Context::init_symbol(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, name, ToString(context, "Context"));
    auto symbol = v8::Private::New(isolate, name);

    auto it = per_isolate.find(isolate);
    it->second.class_symbol.Reset(isolate, symbol);

    return v8::JustVoid();
}

v8::Local<v8::FunctionTemplate> Context::get_template(v8::Isolate *isolate) {
    if (auto it = per_isolate.find(isolate); it != per_isolate.end()) {
        return it->second.class_template.Get(isolate);
    }
    return v8::Local<v8::FunctionTemplate>();
}

v8::Local<v8::Private> Context::get_symbol(v8::Isolate *isolate) {
    if (auto it = per_isolate.find(isolate); it != per_isolate.end()) {
        return it->second.class_symbol.Get(isolate);
    }
    return v8::Local<v8::Private>();
}

void Context::throw_illegal_constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();

    JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
}

void Context::Constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();

    if (!info.IsConstructCall()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }

    auto context_template = get_template(isolate);

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
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "name");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'name' to be a string");
            }
            context_name = js_value.As<v8::String>();
        }
    }

    auto global_constructor_template = v8::FunctionTemplate::New(isolate, throw_illegal_constructor, holder);
    if (!context_name.IsEmpty()) {
        global_constructor_template->SetClassName(context_name);
    }
    global_constructor_template->InstanceTemplate()->SetAccessCheckCallback(access_check, holder);

    auto context_symbol = get_symbol(isolate);

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
    if (auto it = per_isolate.find(isolate); it != per_isolate.end()) {
        std::lock_guard lock(it->second.active_context_lock);
        it->second.active_context.insert(wrapper);
    }

    context->GetMicrotaskQueue()->AddMicrotasksCompletedCallback(
        reinterpret_cast<v8::MicrotasksCompletedCallbackWithData>(creation_context_after_microtasks_completed),
        wrapper
    );

    info.GetReturnValue().Set(info.This());
}

void Context::at_exit(v8::Isolate *isolate) {
    on_isolate_dispose(isolate);
}

void Context::on_isolate_dispose(v8::Isolate *isolate) {
    auto it = per_isolate.find(isolate);
    if (it == per_isolate.end()) {
        return;
    }
    v8::Isolate::Scope isolate_scope(isolate);
    v8::Locker isolate_locker(isolate);
    v8::HandleScope scope(isolate);
    std::lock_guard lock(it->second.active_context_lock);
    per_isolate_value &pvi = it->second;
    while (!pvi.active_context.empty()) {
        auto wrapper = *(pvi.active_context.begin());
        wrapper->m_task_queue->RemoveMicrotasksCompletedCallback(
            reinterpret_cast<v8::MicrotasksCompletedCallbackWithData>(creation_context_after_microtasks_completed),
            wrapper
        );
        delete wrapper;
    }
    per_isolate.erase(it);
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
    auto isolate = m_isolate;
    v8::Isolate::Scope isolate_scope(isolate);
    v8::Locker isolate_locker(isolate);
    v8::HandleScope scope(isolate);

    if (auto it = per_isolate.find(m_isolate); it != per_isolate.end()) {
        std::lock_guard lock(it->second.active_context_lock);
        it->second.active_context.erase(this);
    }

    auto holder = this->persistent().Get(isolate);
    if (!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1 && holder->GetAlignedPointerFromInternalField(0) == this) {
        holder->SetAlignedPointerInInternalField(0, nullptr);
    }
}

void Context::creation_context_after_microtasks_completed(v8::Isolate *isolate, Context *wrapper) {
    auto it = per_isolate.find(isolate);
    if (it == per_isolate.end()) {
        return;
    }
    {
        std::lock_guard lock(it->second.active_context_lock);
        if (!it->second.active_context.contains(wrapper)) {
            return;
        }
    }
    if (!wrapper->auto_run_microtask_queue) {
        return;
    }
    auto context = wrapper->m_context.Get(isolate);
    context->GetMicrotaskQueue()->PerformCheckpoint(isolate);
}

bool Context::access_check(v8::Local<v8::Context> accessing_context, v8::Local<v8::Object> accessed_object, v8::Local<v8::Value> data) {
    auto isolate = accessing_context->GetIsolate();
    v8::HandleScope scope(isolate);

    if (auto it = per_isolate.find(isolate); it != per_isolate.end()) {
        auto wrapper = node::ObjectWrap::Unwrap<Context>(data.As<v8::Object>());
        {
            std::lock_guard lock(it->second.active_context_lock);
            if (!it->second.active_context.contains(wrapper)) {
                return false;
            }
        }
        auto control_context = wrapper->m_control_context.Get(isolate);
        if (control_context->Global()->SameValue(accessing_context->Global())) {
            return true;
        }
    }
    return false;
}

void Context::NativeFunction(const v8::FunctionCallbackInfo<v8::Value> &info) {
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
}

void Context::native_function_callback(const v8::FunctionCallbackInfo<v8::Value> &info) {
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

    if (auto it = per_isolate.find(isolate); it != per_isolate.end()) {
        auto context_symbol = it->second.class_symbol.Get(isolate);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, holder_value, context->Global()->GetPrivate(context, context_symbol));
        if (holder_value->IsObject()) {
            receiver = holder_value;
        }
    }
    if (receiver.IsEmpty()) {
        receiver = v8::Undefined(isolate);
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, return_value, info.Data().As<v8::Function>()->Call(context, receiver, 3, call_args));
    info.GetReturnValue().Set(return_value);
}

void Context::CompileFunction(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    auto wrapper = node::ObjectWrap::Unwrap<Context>(info.Holder());
    if (wrapper == nullptr) {
        JS_THROW_ERROR(NOTHING, context, ReferenceError, "The context has already been disposed.");
    }

    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected 1 argument, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected argument[0] (options) to be an object.");
    }
    auto options = info[0].As<v8::Object>();

    v8::Local<v8::String> name;
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, value, context, options, "name");
        if (!value->IsNullOrUndefined()) {
            if (!value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'name' to be a string.");
            }
            name = value.As<v8::String>();
        }
    }

    std::vector<v8::Local<v8::String>> arg_names;
    arg_names.reserve(8);
    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "arguments");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsArray()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'arguments' to be an array.");
            }
            auto js_array = js_value.As<v8::Array>();
            for (decltype(js_array->Length()) i = 0; i < js_array->Length(); ++i) {
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, arg_name_value, js_array->Get(context, i));
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
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "scopes");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsArray()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option 'scopes' to be an array.");
            }
            auto js_array = js_value.As<v8::Array>();
            for (decltype(js_array->Length()) i = 0; i < js_array->Length(); ++i) {
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, scope_value, js_array->Get(context, i));
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

void Context::Dispose(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);

    auto wrapper = node::ObjectWrap::Unwrap<Context>(info.Holder());
    if (wrapper == nullptr) {
        // Already disposed, do nothing.
        return;
    }

    if (auto it = per_isolate.find(isolate); it != per_isolate.end()) {
        std::lock_guard lock(it->second.active_context_lock);
        it->second.active_context.erase(wrapper);
    }

    auto wrapped_context = wrapper->m_context.Get(isolate);

    // Delete the wrapped object
    delete wrapper;

    v8::Context::Scope wrapped_context_scope(wrapped_context);
    isolate->ContextDisposedNotification(true);
}

void Context::GetGlobal(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);

    info.GetReturnValue().SetNull();
    auto wrapper = node::ObjectWrap::Unwrap<Context>(info.Holder());
    if (wrapper == nullptr) {
        // Already disposed, do nothing.
        return;
    }

    auto wrapped_context = wrapper->m_context.Get(isolate);
    info.GetReturnValue().Set(wrapped_context->Global());
}
