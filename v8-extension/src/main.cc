#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <v8.h>
#include "js-helper.h"
#include "function.h"
#include "security-token.h"
#include "context.h"

using namespace dragiyski::node_ext;

NODE_MODULE_INIT() {
    context_init(context);
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