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
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "globalOf"));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, v8::Function::New(context, js_global_of, exports, 1, v8::ConstructorBehavior::kThrow));
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
    JS_EXECUTE_IGNORE(NOTHING, Context::Init(context));
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, name, ToString(context, "Context"));
        auto class_template = Context::get_template(context->GetIsolate());
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, value, class_template->GetFunction(context));
        JS_EXECUTE_IGNORE(NOTHING, exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_FROZEN));
    }
}