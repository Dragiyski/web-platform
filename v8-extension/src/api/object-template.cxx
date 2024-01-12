#include "object-template.hxx"

#include "context.hxx"

#include "../js-string-table.hxx"
#include "../error-message.hxx"
#include <map>
#include <vector>

namespace dragiyski::node_ext {
    using namespace js;
    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_template;
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void ObjectTemplate::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = ::js::StringTable::Get(isolate, "FunctionTemplate");
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
        class_template->SetClassName(class_name);

        auto signature = v8::Signature::New(isolate, class_template);
        auto prototype_template = class_template->PrototypeTemplate();
        
        class_template->ReadOnlyPrototype();
        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_class_symbol.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_cache)
        );

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void ObjectTemplate::uninitialize(v8::Isolate* isolate) {
        per_isolate_template.erase(isolate);
        per_isolate_class_symbol.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> ObjectTemplate::get_class_template(v8::Isolate* isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    v8::Local<v8::Private> ObjectTemplate::get_class_symbol(v8::Isolate* isolate) {
        assert(per_isolate_class_symbol.contains(isolate));
        return per_isolate_class_symbol[isolate].Get(isolate);
    }

    v8::Maybe<void> ObjectTemplate::Setup(v8::Local<v8::Context> context, v8::Local<v8::ObjectTemplate> value, v8::Local<v8::Object> settings) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        bool mark_as_undectable = false;
        {
            auto name = StringTable::Get(isolate, "undetectable");
            JS_EXPRESSION_RETURN(js_value, settings->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                mark_as_undectable = js_value->BooleanValue(isolate);
            }
        }

        bool is_code_like = false;
        {
            auto name = StringTable::Get(isolate, "codeLike");
            JS_EXPRESSION_RETURN(js_value, settings->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                is_code_like = js_value->BooleanValue(isolate);
            }
        }
        
        bool immutable_prototype = false;
        {
            auto name = StringTable::Get(isolate, "immutablePrototype");
            JS_EXPRESSION_RETURN(js_value, settings->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                immutable_prototype = js_value->BooleanValue(isolate);
            }
        }

        bool has_named_handler = false;
        v8::NamedPropertyHandlerConfiguration named_handler;
        v8::Local<v8::Value> named_getter, named_setter, named_query, named_deleter, named_enumerator, named_definer, named_descriptor;
        {
            auto name = StringTable::Get(isolate, "namedHandler");
            JS_EXPRESSION_RETURN(js_value, settings->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                if (!js_value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"namedHandler\" is not an object");
                }
                auto value_named_handler = js_value.As<v8::Object>();
                named_handler.flags = v8::PropertyHandlerFlags::kNone;
                {
                    auto name = StringTable::Get(isolate, "shared");
                    JS_EXPRESSION_RETURN(value, value_named_handler->Get(context, name));
                    if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                        named_handler.flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(named_handler.flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kAllCanRead));
                    }
                }
                {
                    auto name = StringTable::Get(isolate, "fallback");
                    JS_EXPRESSION_RETURN(value, value_named_handler->Get(context, name));
                    if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                        named_handler.flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(named_handler.flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kNonMasking));
                    }
                }
                {
                    auto name = StringTable::Get(isolate, "string");
                    JS_EXPRESSION_RETURN(value, value_named_handler->Get(context, name));
                    if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                        named_handler.flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(named_handler.flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kOnlyInterceptStrings));
                    }
                }
                {
                    auto name = StringTable::Get(isolate, "sideEffects");
                    JS_EXPRESSION_RETURN(value, value_named_handler->Get(context, name));
                    if (!value->IsNullOrUndefined() && !value->BooleanValue(isolate)) {
                        named_handler.flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(named_handler.flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kHasNoSideEffect));
                    }
                }
#define OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(variable, property_name) \
{ \
    auto name = StringTable::Get(isolate, property_name); \
    JS_EXPRESSION_RETURN(value, value_named_handler->Get(context, name)); \
    if (!value->IsNullOrUndefined()) { \
        if (value->IsFunction()) { \
            variable = value; \
        } else if (value->IsObject() && value.As<v8::Object>()->IsCallable()) { \
            variable = value; \
        } else { \
            JS_THROW_ERROR(TypeError, isolate, "Option", " ", "\"", "namedHandler", ".", property_name, "\"", " ", "is not an object"); \
        } \
    } \
}
                OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(named_getter, "getter");
                OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(named_setter, "setter");
                OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(named_query, "query");
                OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(named_deleter, "deleter");
                OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(named_enumerator, "enumerator");
                OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(named_definer, "definer");
                OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT(named_descriptor, "descriptor");
#undef OBJECT_TEMPLATE_SETUP_GET_HANDLER_CALLBACK_FROM_OBJECT
            }
        }

        v8::Local<v8::Object> properties;
        {
            auto name = StringTable::Get(isolate, "properties");
            JS_EXPRESSION_RETURN(value, settings->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"properties\" is not an object");
                }
            }
        }
    }

    v8::Maybe<void> ObjectTemplate::ConfigureTemplate(v8::Local<v8::Context> context, v8::Local<v8::ObjectTemplate> value, v8::Local<v8::Object> settings) {
        static const constexpr auto __function_return_type__ = v8::Nothing<void>;
        auto isolate = context->GetIsolate();
        v8::HandleScope scope(isolate);

        bool mark_as_undectable = false;
        {
            auto name = StringTable::Get(isolate, "undetectable");
            JS_EXPRESSION_RETURN(js_value, settings->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                mark_as_undectable = js_value->BooleanValue(isolate);
            }
        }

        bool is_code_like = false;
        {
            auto name = StringTable::Get(isolate, "codeLike");
            JS_EXPRESSION_RETURN(js_value, settings->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                is_code_like = js_value->BooleanValue(isolate);
            }
        }
        
        bool immutable_prototype = false;
        {
            auto name = StringTable::Get(isolate, "immutablePrototype");
            JS_EXPRESSION_RETURN(js_value, settings->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                immutable_prototype = js_value->BooleanValue(isolate);
            }
        }

        v8::Local<v8::Object> properties;
        {
            auto name = StringTable::Get(isolate, "properties");
            JS_EXPRESSION_RETURN(value, settings->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"properties\" is not an object");
                }
            }
        }
    }
}
