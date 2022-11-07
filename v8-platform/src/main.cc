#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
#include "js-helper.h"

void js_get_security_token(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
    }
    auto object = info[0].As<v8::Object>();
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, object_context, object->GetCreationContext());
    auto security_token = object_context->GetSecurityToken();
    info.GetReturnValue().Set(security_token);
}

void js_set_security_token(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 2) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
    }
    auto object = info[0].As<v8::Object>();
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, object_context, object->GetCreationContext());
    auto security_token = object_context->GetSecurityToken();
    info.GetReturnValue().Set(security_token);
    object_context->SetSecurityToken(info[1]);
}

void js_use_default_security_token(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
    }
    auto object = info[0].As<v8::Object>();
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, object_context, object->GetCreationContext());
    auto security_token = object_context->GetSecurityToken();
    info.GetReturnValue().Set(security_token);
    object_context->UseDefaultSecurityToken();
}

void v8_function_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    v8::Local<v8::Array> call_arguments;
    {
        v8::Local<v8::Value> array_arguments[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            array_arguments[i] = info[i];
        }
        call_arguments = v8::Array::New(isolate, array_arguments, info.Length());
    }
    auto call_this = info.This();
    auto call_target = info.NewTarget();
    v8::Local<v8::Value> args[] = { call_this, call_arguments, call_target };
    v8::MaybeLocal<v8::Value> maybe_result;
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, result, info.Data().As<v8::Function>()->Call(context, v8::Undefined(isolate), 3, args));
    info.GetReturnValue().Set(result);
}
/**
 * @brief Create a native function in a context.
 *
 * @param info
 * @param info[0] {Object} context
 * @param info[1] {Function} function: Function to call when this function is invoked.
 * @param info[2] {string} name='': The name of the function.
 * @param info[3] {boolean}=true constructor: Can this function be called as constructor.
 * @param info[4] {Number}=0 length: Minimum number required for the function.
 */
void js_create_native_function(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto caller_context = isolate->GetCurrentContext();
    if (info.Length() < 2) {
        JS_THROW_ERROR(NOTHING, caller_context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, caller_context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
    }
    if (!info[1]->IsFunction()) {
        JS_THROW_ERROR(NOTHING, caller_context, TypeError, "Expected arguments[1] to be a function, got ", info[1]);
    }
    v8::Local<v8::String> function_name;
    if (!info[2]->IsNullOrUndefined()) {
        if (!info[2]->IsString()) {
            JS_THROW_ERROR(NOTHING, caller_context, TypeError, "Expected arguments[2] to be a string, got ", info[2]);
        }
        function_name = info[2].As<v8::String>();
    }
    auto constructor_behavior = v8::ConstructorBehavior::kAllow;
    if (!info[3]->IsNullOrUndefined()) {
        auto allow_constructor = info[3]->BooleanValue(isolate);
        if (!allow_constructor) {
            constructor_behavior = v8::ConstructorBehavior::kThrow;
        }
    }
    int length = 0;
    if (!info[4]->IsNullOrUndefined()) {
        JS_EXECUTE_RETURN(NOTHING, int, value, info[4]->Int32Value(caller_context));
        length = value;
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, callee_context, info[0].As<v8::Object>()->GetCreationContext());
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, callee, v8::Function::New(callee_context, v8_function_callback, info[1], length, constructor_behavior));
    if (!function_name.IsEmpty()) {
        callee->SetName(function_name);
    }
    info.GetReturnValue().Set(callee);
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

void js_function_set_name(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 2) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
    }
    if (!info[0]->IsFunction()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
    }
    if (!info[1]->IsString()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[1] to be a string, got ", info[1]);
    }
    info[0].As<v8::Function>()->SetName(info[1].As<v8::String>());
}

void js_function_get_name(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
    }
    if (!info[0]->IsFunction()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
    }
    info.GetReturnValue().Set(info[0].As<v8::Function>()->GetName());
}

bool v8_context_access_check(v8::Local<v8::Context> accessing_context, v8::Local<v8::Object> accessed_object, v8::Local<v8::Value> value) {
    return accessing_context->Global()->SameValue(value);
}

void js_create_context(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto current_context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsString()) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
    }
    auto name = info[0].As<v8::String>();
    auto current_global = current_context->Global();
    auto class_template = v8::FunctionTemplate::New(isolate);
    class_template->SetClassName(name);
    // WARNING: Don't install access checks on the prototype template, as it breaks V8 DCHECK
    class_template->InstanceTemplate()->SetAccessCheckCallback(v8_context_access_check, current_global);
    // The new context MUST be created with the current context microtask queue, as all executions
    // in the new context are synchronous with the current context.
    auto context = v8::Context::New(
        isolate,
        nullptr,
        class_template->InstanceTemplate(),
        v8::Local<v8::Value>(),
        v8::DeserializeInternalFieldsCallback(),
        current_context->GetMicrotaskQueue()
    );
    info.GetReturnValue().Set(context->Global());
}

/**
 * @brief Compile string to a function within a context.
 *
 * @param info
 * @param info[0] {Object} Context
 * @param info[1] {Array<String>} Argument Names (must be valid javascript identifiers)
 * @param info[2] {Array<Object>} Context extension (must be objects)
 * @param info[3] {String} Source Code
 * @param info[3] {Object} options
 */
void js_compile_function(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto current_context = isolate->GetCurrentContext();
    if (info.Length() < 4) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected ", 3, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
    }
    if (!info[1]->IsArray()) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[1] to be an array, got ", info[0]);
    }
    if (!info[2]->IsArray()) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[2] to be an array, got ", info[0]);
    }
    if (!info[3]->IsString()) {
        JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[3] to be a string, got ", info[0]);
    }
    v8::Local<v8::Object> options;
    if (info.Length() > 4 && !info[4]->IsNullOrUndefined()) {
        if (!info[4]->IsObject()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected optional arguments[3] to be an object, got ", info[3]);
        }
        options = info[4].As<v8::Object>();
    } else {
        options = v8::Object::New(isolate);
    }

    v8::Local<v8::Context> creation_context = current_context;
    {
        auto value = info[0].As<v8::Object>()->GetCreationContext();
        if (!value.IsEmpty()) {
            creation_context = value.ToLocalChecked();
        }
    }

    decltype(info[1].As<v8::Array>()->Length()) arg_length = info[1].As<v8::Array>()->Length();
    v8::Local<v8::String> args[arg_length];
    for (decltype(arg_length) i = 0; i < arg_length; ++i) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_arg, info[1].As<v8::Array>()->Get(current_context, i));
        if (!js_arg->IsString()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "arguments[1][", i, "]: expected a string, got ", js_arg);
        }
    }
    decltype(info[2].As<v8::Array>()->Length()) ctx_length = info[2].As<v8::Array>()->Length();
    v8::Local<v8::Object> ctxs[ctx_length];
    for (decltype(ctx_length) i = 0; i < ctx_length; ++i) {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_ctx, info[2].As<v8::Array>()->Get(current_context, i));
        if (!js_ctx->IsObject()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "arguments[1][", i, "]: expected a string, got ", js_ctx);
        }
    }

    v8::Local<v8::Value> location;
    int line_offset = 0, column_offset = 0;
    v8::Local<v8::Value> source_map_url;
    int script_id = -1;
    bool is_shared_cross_origin = false, is_opaque = false, is_wasm = false, is_module = false;

    {
        JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "location");
        if (!js_value->IsNullOrUndefined()) {
            location = js_value;
        }
    }

    if (!location.IsEmpty()) {
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "lineOffset");
            if (!js_value->IsNullOrUndefined()) {
                JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(current_context));
                line_offset = value;
            }
        }
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "columnOffset");
            if (!js_value->IsNullOrUndefined()) {
                JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(current_context));
                column_offset = value;
            }
        }
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "scriptId");
            if (!js_value->IsNullOrUndefined()) {
                JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(current_context));
                script_id = value;
            }
        }
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "sourceMapUrl");
            if (!js_value->IsNullOrUndefined()) {
                source_map_url = js_value;
            }
        }
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isSharedCrossOrigin");
            if (!js_value->IsNullOrUndefined()) {
                is_shared_cross_origin = js_value->BooleanValue(isolate);
            }
        }
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isOpaque");
            if (!js_value->IsNullOrUndefined()) {
                is_opaque = js_value->BooleanValue(isolate);
            }
        }
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isWASM");
            if (!js_value->IsNullOrUndefined()) {
                is_wasm = js_value->BooleanValue(isolate);
            }
        }
        {
            JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isModule");
            if (!js_value->IsNullOrUndefined()) {
                is_module = js_value->BooleanValue(isolate);
            }
        }
        v8::ScriptOrigin origin(isolate, location, line_offset, column_offset, is_shared_cross_origin, script_id, source_map_url, is_opaque, is_wasm, is_module);
        v8::ScriptCompiler::Source source(info[3].As<v8::String>(), origin);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, result, v8::ScriptCompiler::CompileFunction(creation_context, &source, arg_length, args, ctx_length, ctxs, v8::ScriptCompiler::kEagerCompile));
        info.GetReturnValue().Set(result);
    } else {
        v8::ScriptCompiler::Source source(info[3].As<v8::String>());
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, result, v8::ScriptCompiler::CompileFunction(creation_context, &source, arg_length, args, ctx_length, ctxs, v8::ScriptCompiler::kEagerCompile));
        info.GetReturnValue().Set(result);
    }
}

class Script : public node::ObjectWrap {
public:
    static v8::Maybe<void> Export(v8::Local<v8::Context> context, v8::Local<v8::Object> exports) {
        auto isolate = context->GetIsolate();
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::FunctionTemplate, function_template, v8::FunctionTemplate::New(isolate, Constructor, exports, v8::Local<v8::Signature>(), 2));
        function_template->InstanceTemplate()->SetInternalFieldCount(1);
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, class_name, ToString(context, "Script"));
        function_template->SetClassName(class_name);
        {
            JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::String, name, ToString(context, "run"));
            auto signature = v8::Signature::New(isolate, function_template);
            JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::FunctionTemplate, value, v8::FunctionTemplate::New(isolate, Run, exports, signature, 1, v8::ConstructorBehavior::kThrow));
            function_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::Function, function, function_template->GetFunction(context));
        JS_EXECUTE_IGNORE(VOID_NOTHING, exports->DefineOwnProperty(context, class_name, function, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }

    static void Constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto current_context = isolate->GetCurrentContext();
        if (!info.IsConstructCall()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Class constructor Script cannot be invoked without 'new'");
        }
        if (info.This()->InternalFieldCount() < 1) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Illegal constructor");
        }
        if (info.Length() < 2) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected ", 3, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
        }
        if (!info[1]->IsString()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[1] to be a string, got ", info[1]);
        }
        v8::Local<v8::Object> options;
        if (!info[2]->IsNullOrUndefined()) {
            if (!info[2]->IsObject()) {
                JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected optional arguments[2] to be an object, got ", info[2]);
            }
            options = info[2].As<v8::Object>();
        }
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, compile_context, info[0].As<v8::Object>()->GetCreationContext());
        v8::Local<v8::Script> script;
        {
            v8::Local<v8::Value> location;
            int line_offset = 0, column_offset = 0;
            v8::Local<v8::Value> source_map_url;
            int script_id = -1;
            bool is_shared_cross_origin = false, is_opaque = false, is_wasm = false, is_module = false;

            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "location");
                if (!js_value->IsNullOrUndefined()) {
                    location = js_value;
                }
            }
            if (!location.IsEmpty()) {
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "lineOffset");
                    if (!js_value->IsNullOrUndefined()) {
                        JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(current_context));
                        line_offset = value;
                    }
                }
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "columnOffset");
                    if (!js_value->IsNullOrUndefined()) {
                        JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(current_context));
                        column_offset = value;
                    }
                }
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "scriptId");
                    if (!js_value->IsNullOrUndefined()) {
                        JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(current_context));
                        script_id = value;
                    }
                }
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "sourceMapUrl");
                    if (!js_value->IsNullOrUndefined()) {
                        source_map_url = js_value;
                    }
                }
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isSharedCrossOrigin");
                    if (!js_value->IsNullOrUndefined()) {
                        is_shared_cross_origin = js_value->BooleanValue(isolate);
                    }
                }
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isOpaque");
                    if (!js_value->IsNullOrUndefined()) {
                        is_opaque = js_value->BooleanValue(isolate);
                    }
                }
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isWASM");
                    if (!js_value->IsNullOrUndefined()) {
                        is_wasm = js_value->BooleanValue(isolate);
                    }
                }
                {
                    JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, current_context, options, "isModule");
                    if (!js_value->IsNullOrUndefined()) {
                        is_module = js_value->BooleanValue(isolate);
                    }
                }
                v8::ScriptOrigin origin(isolate, location, line_offset, column_offset, is_shared_cross_origin, script_id, source_map_url, is_opaque, is_wasm, is_module);
                v8::ScriptCompiler::Source source(info[1].As<v8::String>(), origin);
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Script, script_value, v8::ScriptCompiler::Compile(compile_context, &source, v8::ScriptCompiler::kEagerCompile));
                script = script_value;
            } else {
                v8::ScriptCompiler::Source source(info[1].As<v8::String>());
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Script, script_value, v8::ScriptCompiler::Compile(compile_context, &source, v8::ScriptCompiler::kEagerCompile));
                script = script_value;
            }
        }
        auto wrapper = new Script(current_context, script);
        wrapper->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }

    static void Run(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto current_context = isolate->GetCurrentContext();
        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
        }
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, run_context, info[0].As<v8::Object>()->GetCreationContext());
        auto wrapper = node::ObjectWrap::Unwrap<Script>(info.Holder());
        auto script = wrapper->m_script.Get(isolate);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, return_value, script->Run(run_context));
        info.GetReturnValue().Set(return_value);
    }
private:
    v8::Persistent<v8::Script> m_script;

protected:
    Script(
        v8::Local<v8::Context> current_context,
        v8::Local<v8::Script> script
    ) {
        auto isolate = current_context->GetIsolate();
        m_script.Reset(isolate, script);
    }
};

NODE_MODULE_INIT() {
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "getSecurityToken"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_get_security_token, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "setSecurityToken"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_set_security_token, exports, 2, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "useDefaultSecurityToken"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_use_default_security_token, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "createNativeFunction"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_create_native_function, exports, 2, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "globalOf"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_global_of, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "createContext"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_create_context, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "compileFunction"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_compile_function, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "getFunctionName"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_function_get_name, exports, 1, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "setFunctionName"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_function_set_name, exports, 2, v8::ConstructorBehavior::kThrow));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    JS_EXECUTE_IGNORE(NOTHING, Script::Export(context, exports));
}
