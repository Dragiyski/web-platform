#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <v8.h>
#include "js-helper.h"
#include "function.h"
#include "security-token.h"
#include "native-function.h"
#include "compile-function.h"
#include "context.h"

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
            value->SetClassName(name);
            function_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        JS_EXECUTE_RETURN_HANDLE(VOID_NOTHING, v8::Function, function, function_template->GetFunction(context));
        JS_EXECUTE_IGNORE(VOID_NOTHING, exports->DefineOwnProperty(context, class_name, function, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }

    static void Constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
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
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
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

    static void Run(const v8::FunctionCallbackInfo<v8::Value> &info) {
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
        {
            v8::Isolate::SafeForTerminationScope script_scope(isolate);
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, return_value, script->Run(run_context));
            info.GetReturnValue().Set(return_value);
        }
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

void v8_throw_illegal_constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto current_context = isolate->GetCurrentContext();
    JS_THROW_ERROR(NOTHING, current_context, TypeError, "Illegal constructor");
}

v8::Eternal<v8::FunctionTemplate> handle_module_template;
v8::Eternal<v8::FunctionTemplate> handle_source_module_template;

v8::MaybeLocal<v8::FunctionTemplate> ModuleTemplate(v8::Local<v8::Context> context, v8::Local<v8::Object> exports) {
    auto isolate = context->GetIsolate();
    JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::FunctionTemplate, function_template, v8::FunctionTemplate::New(isolate, v8_throw_illegal_constructor, exports, v8::Local<v8::Signature>(), 0));
    JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::String, class_name, ToString(context, "Module"));
    function_template->SetClassName(class_name);
    handle_module_template.Set(isolate, function_template);
}

class SourceModule : public node::ObjectWrap {
public:
    static v8::MaybeLocal<v8::FunctionTemplate> Template(v8::Local<v8::Context> context, v8::Local<v8::Object> exports, v8::Local<v8::FunctionTemplate> parent) {
        auto isolate = context->GetIsolate();
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::FunctionTemplate, function_template, v8::FunctionTemplate::New(isolate, Constructor, exports, v8::Local<v8::Signature>(), 2));
        function_template->InstanceTemplate()->SetInternalFieldCount(1);
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::String, class_name, ToString(context, "SourceModule"));
        function_template->SetClassName(class_name);
        function_template->Inherit(parent);
        JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::Function, function, function_template->GetFunction(context));
        JS_EXECUTE_IGNORE(JS_NOTHING(v8::FunctionTemplate), exports->DefineOwnProperty(context, class_name, function, JS_PROPERTY_ATTRIBUTE_FROZEN));
        handle_source_module_template.Set(isolate, function_template);
    }

    static void Constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto current_context = isolate->GetCurrentContext();
        if (!info.IsConstructCall()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Class constructor SourceModule cannot be invoked without 'new'");
        }
        if (info.This()->InternalFieldCount() < 1) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Illegal constructor");
        }
        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[1]->IsString()) {
            JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected arguments[0] to be a string, got ", info[1]);
        }
        v8::Local<v8::Object> options;
        if (!info[1]->IsNullOrUndefined()) {
            if (!info[1]->IsObject()) {
                JS_THROW_ERROR(NOTHING, current_context, TypeError, "Expected optional arguments[2] to be an object, got ", info[2]);
            }
            options = info[1].As<v8::Object>();
        }
        v8::Local<v8::Module> module;
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
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Module, module_value, v8::ScriptCompiler::CompileModule(isolate, &source, v8::ScriptCompiler::kEagerCompile));
                module = module_value;
            } else {
                v8::ScriptCompiler::Source source(info[1].As<v8::String>());
                JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Module, module_value, v8::ScriptCompiler::CompileModule(isolate, &source, v8::ScriptCompiler::kEagerCompile));
                module = module_value;
            }
        }
        {
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_specifier, ToString(current_context, "specifier"));
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_request_list, ToString(current_context, "requestList"));
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_source_offset, ToString(current_context, "sourceOffset"));
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_assertions, ToString(current_context, "assertions"));
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_value, ToString(current_context, "value"));
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_name, ToString(current_context, "name"));
            auto request_fixed_array = module->GetModuleRequests();
            v8::Local<v8::Value> request_array[request_fixed_array->Length()];
            for (decltype(request_fixed_array->Length()) i = 0; i < request_fixed_array->Length(); ++i) {
                v8::Local<v8::ModuleRequest> req = request_fixed_array->Get(current_context, i).As<v8::ModuleRequest>();
                auto assertion_fixed_array = req->GetImportAssertions();
                v8::Local<v8::Value> assertion_array[assertion_fixed_array->Length() / 3];
                int current_index = 0;
                for (decltype(assertion_fixed_array->Length()) i = 0; i < assertion_fixed_array->Length(); i += 3) {
                    v8::Local<v8::Name> keys[] = { string_name, string_value, string_source_offset };
                    v8::Local<v8::Value> values[] = {
                        assertion_fixed_array->Get(current_context, i).As<v8::String>(),
                        assertion_fixed_array->Get(current_context, i + 1).As<v8::Value>(),
                        assertion_fixed_array->Get(current_context, i + 2).As<v8::Value>()
                    };
                    auto assertion_object = v8::Object::New(isolate, v8::Null(isolate), keys, values, 3);
                    assertion_object->SetIntegrityLevel(current_context, v8::IntegrityLevel::kFrozen);
                    assertion_array[current_index++] = assertion_object;
                }
                auto assertion_js_array = v8::Array::New(isolate, assertion_array, current_index);
                assertion_js_array->SetIntegrityLevel(current_context, v8::IntegrityLevel::kFrozen);
                v8::Local<v8::Name> keys[] = { string_specifier, string_source_offset, string_assertions };
                auto offset = v8::Int32::New(isolate, req->GetSourceOffset());
                v8::Local<v8::Value> values[] = { req->GetSpecifier(), offset, assertion_js_array };
                auto req_object = v8::Object::New(isolate, v8::Null(isolate), keys, values, 3);
                req_object->SetIntegrityLevel(current_context, v8::IntegrityLevel::kFrozen);
                request_array[i] = req_object;
            }
            auto request_js_array = v8::Array::New(isolate, request_array, request_fixed_array->Length());
            request_js_array->SetIntegrityLevel(current_context, v8::IntegrityLevel::kFrozen);
            JS_EXECUTE_IGNORE(NOTHING, info.This()->DefineOwnProperty(current_context, string_request_list, request_js_array, JS_PROPERTY_ATTRIBUTE_CONSTANT));
        }
        auto wrapper = new SourceModule(current_context, module);
        wrapper->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }

    static v8::MaybeLocal<v8::Module> extractModuleFromObject(v8::Local<v8::Context> context, v8::Local<v8::Object> object) {
        auto isolate = context->GetIsolate();
        v8::EscapableHandleScope scope(isolate);
        auto source_module_template = handle_source_module_template.Get(isolate);
        {
            auto holder = object->FindInstanceInPrototypeChain(source_module_template);
            if (!holder.IsEmpty() && holder->IsObject()) {
                auto wrapper = node::ObjectWrap::Unwrap<SourceModule>(holder);
                return scope.Escape(wrapper->module(isolate));
            }
        }
        JS_THROW_ERROR(JS_NOTHING(v8::Module), context, TypeError, "Cannot convert to 'Module'");
    }

    static void SetRequestModule(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();
        if (info.Length() < 2) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
        }
        if (!info[0]->IsInt32()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an integer, got ", info[0]);
        }
        if (!info[1]->IsObject()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[1] to be an object, got ", info[1]);
        }
        auto source_module_template = handle_source_module_template.Get(isolate);
        auto wrapper = node::ObjectWrap::Unwrap<SourceModule>(info.Holder());
        auto current_module = wrapper->m_module.Get(isolate);
        if (current_module->GetStatus() != v8::Module::Status::kUninstantiated) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Invalid state: this module is already linked");
        }
        JS_EXECUTE_RETURN(NOTHING, int, index, info[0]->Int32Value(context));
        if (wrapper->m_response_list.size() == 0) {
            JS_THROW_ERROR(NOTHING, context, RangeError, "This module has no requests");
        }
        if (index < 0 || index >= wrapper->m_response_list.size()) {
            JS_THROW_ERROR(NOTHING, context, RangeError, "Expected arguments[0] to be an integer in range 0 and ", int(wrapper->m_response_list.size() - 1), ", got ", index);
        }
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Module, response_module, extractModuleFromObject(context, info[1].As<v8::Object>()));
    }

private:
    v8::Persistent<v8::Module> m_module;
    std::vector<v8::Global<v8::Module>> m_response_list;

public:
    v8::Local<v8::Module> module(v8::Isolate *isolate) {
        return m_module.Get(isolate);
    }

protected:
    SourceModule(
        v8::Local<v8::Context> current_context,
        v8::Local<v8::Module> module
    ) {
        auto isolate = current_context->GetIsolate();
        m_module.Reset(isolate, module);
        m_response_list.resize(module->GetModuleRequests()->Length());
    }
};

NODE_MODULE_INIT() {
    auto isolate = context->GetIsolate();
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
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "createNativeFunction"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_create_native_function, exports, 1, v8::ConstructorBehavior::kThrow));
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
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::FunctionTemplate, module_template, ModuleTemplate(context, exports));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::FunctionTemplate, source_module_template, SourceModule::Template(context, exports, module_template));
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "Module"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, class_function, module_template->GetFunction(context));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, class_function, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "SourceModule"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, class_function, module_template->GetFunction(context));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, class_function, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
}