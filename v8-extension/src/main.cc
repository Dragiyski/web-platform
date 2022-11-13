#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <v8.h>
#include "js-helper.h"
#include "function.h"
#include "security-token.h"
#include "api/function-template.h"
#include "api/object-template.h"
#include "api/private.h"

using namespace dragiyski::node_ext;

namespace dragiyski::node_ext {
    void at_exit(v8::Isolate *isolate) {
        ObjectTemplate::uninitialize(isolate);
        FunctionTemplate::uninitialize(isolate);
        Template::uninitialize(isolate);
        Private::uninitialize(isolate);
    }

    void on_isolate_finished(v8::Isolate *isolate) {
        at_exit(isolate);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
NODE_MODULE_INIT() {
#pragma GCC diagnostic pop
    auto isolate = context->GetIsolate();
    JS_EXECUTE_IGNORE(NOTHING, Private::initialize(isolate));
    JS_EXECUTE_IGNORE(NOTHING, Template::initialize(context->GetIsolate()));
    JS_EXECUTE_IGNORE(NOTHING, FunctionTemplate::initialize(context->GetIsolate()));
    JS_EXECUTE_IGNORE(NOTHING, ObjectTemplate::initialize(context->GetIsolate()));
    {
        // Storing values into per_isolate map is really convenient. But since isolate is raw pointer,
        // those maps does not clean it automatically if the isolate is disposed, allowing
        // persistent handles to survive as pointer to freed memory after the memory cleanup.
        // When the application finally exits, those handles are destroyed by freeing already freed memory,
        // causing segmentation fault. As such, we must ensure any per-Isolate map is empty before
        // the application exits. Even better, we must not keep the handles when the Isolate is about to be destroyed.
        auto env = node::GetCurrentEnvironment(context);
        auto platform = node::GetMultiIsolatePlatform(env);
        // In case this is a temporary isolate where the platform might survive after cleanup.
        platform->AddIsolateFinishedCallback(isolate, reinterpret_cast<void(*)(void*)>(on_isolate_finished), isolate);
        // In case main environment exists. This might eventually call the isolate finished callback,
        // but in this case, but if it is async, it might be called after the Isolate is disposed,
        // which is what causing the problem in the first place.
        node::AtExit(env, reinterpret_cast<void(*)(void*)>(at_exit), isolate);
    }
    {
        auto name = Private::get_name(isolate);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, Private::get_template(isolate)->GetFunction(context));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        auto name = Template::get_name(isolate);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, Template::get_template(isolate)->GetFunction(context));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        auto name = FunctionTemplate::get_name(isolate);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, FunctionTemplate::get_template(isolate)->GetFunction(context));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
    {
        auto name = ObjectTemplate::get_name(isolate);
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, ObjectTemplate::get_template(isolate)->GetFunction(context));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
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
    // {
    //     JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "globalOf"));
    //     JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_global_of, exports, 1, v8::ConstructorBehavior::kThrow));
    //     JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    // }
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
}