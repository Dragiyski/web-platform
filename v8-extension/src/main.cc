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
#include "api/context.h"
#include "api/user-context.h"
#include "object.h"
#include "string-table.h"

using namespace dragiyski::node_ext;

namespace dragiyski::node_ext {
    void at_exit(v8::Isolate *isolate) {
        UserContext::uninitialize(isolate);
        Context::uninitialize(isolate);
        ObjectTemplate::uninitialize(isolate);
        FunctionTemplate::uninitialize(isolate);
        Template::uninitialize(isolate);
        Private::uninitialize(isolate);
        ObjectWrap::uninitialize(isolate);
        string_map::uninitialize(isolate);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
NODE_MODULE_INIT() {
#pragma GCC diagnostic pop
    auto isolate = context->GetIsolate();
    string_map::initialize(isolate);
    JS_EXECUTE_IGNORE(NOTHING, ObjectWrap::initialize(isolate));
    JS_EXECUTE_IGNORE(NOTHING, Private::initialize(isolate));
    JS_EXECUTE_IGNORE(NOTHING, Template::initialize(context->GetIsolate()));
    JS_EXECUTE_IGNORE(NOTHING, FunctionTemplate::initialize(context->GetIsolate()));
    JS_EXECUTE_IGNORE(NOTHING, ObjectTemplate::initialize(context->GetIsolate()));
    JS_EXECUTE_IGNORE(NOTHING, Context::initialize(context->GetIsolate()));
    JS_EXECUTE_IGNORE(NOTHING, UserContext::initialize(context->GetIsolate()));
    {
        auto env = node::GetCurrentEnvironment(context);
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