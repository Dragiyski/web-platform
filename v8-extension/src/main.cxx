#include <node.h>
#include <v8.h>
#include "js-helper.hxx"
#include "js-string-table.hxx"
#include "api/private.hxx"
#include "api/frozen-map.hxx"
#include "api/context.hxx"
#include "api/function-template.hxx"
#include "api/object-template.hxx"

namespace {
    using callback_t = void (*)(void*);

    v8::Maybe<void> initialize(v8::Local<v8::Context> context) {
        auto isolate = context->GetIsolate();
        js::StringTable::initialize(isolate);
        dragiyski::node_ext::Private::initialize(isolate);
        dragiyski::node_ext::Context::initialize(isolate);
        dragiyski::node_ext::FrozenMap::initialize(isolate);
        dragiyski::node_ext::FunctionTemplate::initialize(isolate);
        dragiyski::node_ext::ObjectTemplate::initialize(isolate);
        return v8::JustVoid();
    }

    void uninitialize(v8::Isolate* isolate) {
        dragiyski::node_ext::ObjectTemplate::uninitialize(isolate);
        dragiyski::node_ext::FunctionTemplate::uninitialize(isolate);
        dragiyski::node_ext::FrozenMap::uninitialize(isolate);
        dragiyski::node_ext::Context::uninitialize(isolate);
        dragiyski::node_ext::Private::uninitialize(isolate);
        js::StringTable::uninitialize(isolate);
    }
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
        auto class_template = Private::get_template(isolate);
        JS_EXPRESSION_RETURN(value, class_template->GetFunction(context));
        JS_EXPRESSION_IGNORE(exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_STATIC));
    }
    {
        auto name = js::StringTable::Get(isolate, "Context");
        auto class_template = Context::get_template(isolate);
        JS_EXPRESSION_RETURN(value, class_template->GetFunction(context));
        JS_EXPRESSION_IGNORE(exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_STATIC));
    }
    {
        auto name = js::StringTable::Get(isolate, "FunctionTemplate");
        auto class_template = FunctionTemplate::get_template(isolate);
        JS_EXPRESSION_RETURN(value, class_template->GetFunction(context));
        JS_EXPRESSION_IGNORE(exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_STATIC));
    }
    {
        v8::Local<v8::Name> names[] = {
            StringTable::Get(isolate, "NONE"),
            StringTable::Get(isolate, "DONT_DELETE"),
            StringTable::Get(isolate, "DONT_ENUM"),
            StringTable::Get(isolate, "READ_ONLY")
        };
        v8::Local<v8::Value> values[] = {
            v8::Integer::New(isolate, v8::PropertyAttribute::None),
            v8::Integer::New(isolate, v8::PropertyAttribute::DontDelete),
            v8::Integer::New(isolate, v8::PropertyAttribute::DontEnum),
            v8::Integer::New(isolate, v8::PropertyAttribute::ReadOnly)
        };
        auto value = v8::Object::New(isolate, v8::Null(isolate), names, values, 4);
        JS_EXPRESSION_IGNORE(value->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen));
        auto name = StringTable::Get(isolate, "propertyAttribute");
        JS_EXPRESSION_IGNORE(exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_STATIC));
    }

    {
        v8::Local<v8::Name> names[] = {
            StringTable::Get(isolate, "HAS_NO_SIDE_EFFECTS"),
            StringTable::Get(isolate, "HAS_SIDE_EFFECTS"),
            StringTable::Get(isolate, "HAS_SIDE_EFFECTS_TO_RECEIVER")
        };
        v8::Local<v8::Value> values[] = {
            v8::Integer::New(isolate, static_cast<int32_t>(v8::SideEffectType::kHasNoSideEffect)),
            v8::Integer::New(isolate, static_cast<int32_t>(v8::SideEffectType::kHasSideEffect)),
            v8::Integer::New(isolate, static_cast<int32_t>(v8::SideEffectType::kHasSideEffectToReceiver))
        };
        auto value = v8::Object::New(isolate, v8::Null(isolate), names, values, 3);
        JS_EXPRESSION_IGNORE(value->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen));
        auto name = StringTable::Get(isolate, "sideEffectType");
        JS_EXPRESSION_IGNORE(exports->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_STATIC));
    }
}