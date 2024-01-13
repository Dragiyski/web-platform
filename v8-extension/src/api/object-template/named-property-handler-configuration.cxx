#include "named-property-handler-configuration.hxx"

#include <cassert>
#include <map>
#include <memory>

#include "../../error-message.hxx"
#include "../../js-string-table.hxx"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_template;
    }

    void ObjectTemplate::NamedPropertyHandlerConfiguration::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_template.contains(isolate));

        auto class_name = StringTable::Get(isolate, "NamedPropertyHandlerConfiguration");
        auto class_template = v8::FunctionTemplate::New(isolate, constructor);
        class_template->SetClassName(class_name);
        auto prototype_template = class_template->PrototypeTemplate();
        auto signature = v8::Signature::New(isolate, class_template);

        // Makes prototype *property* (not object) immutable similar to class X {}; syntax;
        class_template->ReadOnlyPrototype();

        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void ObjectTemplate::NamedPropertyHandlerConfiguration::uninitialize(v8::Isolate* isolate) {
        per_isolate_template.erase(isolate);
    }

    void ObjectTemplate::NamedPropertyHandlerConfiguration::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not an object.");
        }
        auto options = info[0].As<v8::Object>();
        auto target = std::unique_ptr<ObjectTemplate::NamedPropertyHandlerConfiguration>(new ObjectTemplate::NamedPropertyHandlerConfiguration());

        {
            auto name = StringTable::Get(isolate, "shared");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                target->_flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(target->_flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kAllCanRead));
            }
        }
        {
            auto name = StringTable::Get(isolate, "fallback");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                target->_flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(target->_flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kNonMasking));
            }
        }
        {
            auto name = StringTable::Get(isolate, "string");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                target->_flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(target->_flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kOnlyInterceptStrings));
            }
        }
        {
            auto name = StringTable::Get(isolate, "sideEffects");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined() && !value->BooleanValue(isolate)) {
                target->_flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(target->_flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kHasNoSideEffect));
            }
        }
        {
            auto name = StringTable::Get(isolate, "getter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction() || !(value->IsObject() && value.As<v8::Object>()->IsCallable())) {
                    JS_THROW_ERROR(TypeError, isolate, "NamedPropertyHandlerConfiguration.getter specified, but not a function");
                }
                target->_getter.Reset(isolate, value);
            }
        }
    }
}