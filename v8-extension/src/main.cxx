#include <node.h>
#include <v8.h>
#include "js-helper.hxx"
#include "js-string-table.hxx"

using callback_t = void (*)(void *);

CONTEXT_AWARE_FUNCTION(void, TestCallback, (const v8::FunctionCallbackInfo<v8::Value> &info), {
    auto isolate = info.GetIsolate();
    JS_THROW_ERROR(TypeError, isolate, "Error from ", __function_definition__);
});

void at_exit(v8::Isolate *isolate) {
    js::StringTable::uninitialize(isolate);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
NODE_MODULE_INIT() {
#pragma GCC diagnostic pop
    auto isolate = context->GetIsolate();
    v8::HandleScope scope(isolate);
    using __function_return_type__ = void;

    {
        js::StringTable::initialize(isolate);
    }

    auto node_env = node::GetCurrentEnvironment(context);
    node::AtExit(node_env, reinterpret_cast<callback_t>(at_exit), isolate);

    {
        auto name = js::StringTable::Get(isolate, "test_error_function");
        JS_EXPRESSION_RETURN(value, v8::Function::New(
            context,
            TestCallback,
            {},
            0,
            v8::ConstructorBehavior::kThrow
        ));
        value->SetName(name);
        JS_EXPRESSION_IGNORE(exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_STATIC));
    }
}