#include <node.h>
#include <v8.h>
#include "js-helper.hxx"
#include "js-string-table.hxx"
#include "api/private.hxx"

using callback_t = void (*)(void *);

v8::Maybe<void> initialize(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    js::StringTable::initialize(isolate);
    js::Wrapper::initialize(isolate);
    dragiyski::node_ext::Private::initialize(isolate);
    return v8::JustVoid();
}

void uninitialize(v8::Isolate *isolate) {
    dragiyski::node_ext::Private::uninitialize(isolate);
    js::Wrapper::uninitialize(isolate);
    js::StringTable::uninitialize(isolate);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
NODE_MODULE_INIT() {
#pragma GCC diagnostic pop
    using namespace dragiyski::node_ext;
    auto isolate = context->GetIsolate();
    v8::HandleScope scope(isolate);
    using __function_return_type__ = void;

    auto init_result = initialize(context);
    if (init_result.IsNothing()) {
        uninitialize(isolate);
        return;
    }

    auto node_env = node::GetCurrentEnvironment(context);
    node::AtExit(node_env, reinterpret_cast<callback_t>(uninitialize), isolate);

    {
        auto name = js::StringTable::Get(isolate, "Private");
        auto class_template = Private::get_class_template(isolate);
        JS_EXPRESSION_RETURN(value, class_template->GetFunction(context));
        JS_EXPRESSION_IGNORE(exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_STATIC));
    }
}