#include "indexed-property-handler-configuration.hxx"

#include <cassert>
#include <map>
#include <memory>

#include "../../error-message.hxx"
#include "../../js-string-table.hxx"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_template;
    }

    void ObjectTemplate::IndexedPropertyHandlerConfiguration::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_template.contains(isolate));

        auto class_name = StringTable::Get(isolate, "IndexedPropertyHandlerConfiguration");
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

    void ObjectTemplate::IndexedPropertyHandlerConfiguration::uninitialize(v8::Isolate* isolate) {
        per_isolate_template.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_template(v8::Isolate* isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    void ObjectTemplate::IndexedPropertyHandlerConfiguration::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            v8::Local<v8::Value> args[] = { info[0] };
            JS_EXPRESSION_RETURN(callee, get_template(isolate)->GetFunction(context));
            JS_EXPRESSION_RETURN(return_value, callee->NewInstance(context, 1, args));
            info.GetReturnValue().Set(return_value);
            return;
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if (!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not an object.");
        }
        auto options = info[0].As<v8::Object>();
        auto target = std::unique_ptr<ObjectTemplate::IndexedPropertyHandlerConfiguration>(new ObjectTemplate::IndexedPropertyHandlerConfiguration());

        {
            auto name = StringTable::Get(isolate, "shared");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                target->_flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(target->_flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kAllCanRead));
            }
        }
        // "Currently only valid for named interceptors." - If this becomes available for indexed interceptor, uncomment below:
        /* {
            auto name = StringTable::Get(isolate, "fallback");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined() && value->BooleanValue(isolate)) {
                target->_flags = static_cast<v8::PropertyHandlerFlags>(static_cast<unsigned int>(target->_flags) | static_cast<unsigned int>(v8::PropertyHandlerFlags::kNonMasking));
            }
        } */
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
        {
            auto name = StringTable::Get(isolate, "setter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction() || !(value->IsObject() && value.As<v8::Object>()->IsCallable())) {
                    JS_THROW_ERROR(TypeError, isolate, "NamedPropertyHandlerConfiguration.setter specified, but not a function");
                }
                target->_setter.Reset(isolate, value);
            }
        }
        {
            auto name = StringTable::Get(isolate, "query");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction() || !(value->IsObject() && value.As<v8::Object>()->IsCallable())) {
                    JS_THROW_ERROR(TypeError, isolate, "NamedPropertyHandlerConfiguration.query specified, but not a function");
                }
                target->_query.Reset(isolate, value);
            }
        }
        {
            auto name = StringTable::Get(isolate, "deleter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction() || !(value->IsObject() && value.As<v8::Object>()->IsCallable())) {
                    JS_THROW_ERROR(TypeError, isolate, "NamedPropertyHandlerConfiguration.deleter specified, but not a function");
                }
                target->_deleter.Reset(isolate, value);
            }
        }
        {
            auto name = StringTable::Get(isolate, "enumerator");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction() || !(value->IsObject() && value.As<v8::Object>()->IsCallable())) {
                    JS_THROW_ERROR(TypeError, isolate, "NamedPropertyHandlerConfiguration.enumerator specified, but not a function");
                }
                target->_enumerator.Reset(isolate, value);
            }
        }
        {
            auto name = StringTable::Get(isolate, "definer");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction() || !(value->IsObject() && value.As<v8::Object>()->IsCallable())) {
                    JS_THROW_ERROR(TypeError, isolate, "NamedPropertyHandlerConfiguration.definer specified, but not a function");
                }
                target->_definer.Reset(isolate, value);
            }
        }
        {
            auto name = StringTable::Get(isolate, "descriptor");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsFunction() || !(value->IsObject() && value.As<v8::Object>()->IsCallable())) {
                    JS_THROW_ERROR(TypeError, isolate, "NamedPropertyHandlerConfiguration.descriptor specified, but not a function");
                }
                target->_descriptor.Reset(isolate, value);
            }
        }
        info.GetReturnValue().Set(info.This());
    }

    const v8::PropertyHandlerFlags &ObjectTemplate::IndexedPropertyHandlerConfiguration::get_flags() const {
        return _flags;
    }

    v8::Local<v8::Value> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_getter(v8::Isolate *isolate) const {
        return _getter.Get(isolate);
    }

    v8::Local<v8::Value> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_setter(v8::Isolate *isolate) const {
        return _setter.Get(isolate);
    }

    v8::Local<v8::Value> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_query(v8::Isolate *isolate) const {
        return _query.Get(isolate);
    }

    v8::Local<v8::Value> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_deleter(v8::Isolate *isolate) const {
        return _deleter.Get(isolate);
    }

    v8::Local<v8::Value> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_enumerator(v8::Isolate *isolate) const {
        return _enumerator.Get(isolate);
    }

    v8::Local<v8::Value> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_definer(v8::Isolate *isolate) const {
        return _definer.Get(isolate);
    }

    v8::Local<v8::Value> ObjectTemplate::IndexedPropertyHandlerConfiguration::get_descriptor(v8::Isolate *isolate) const {
        return _descriptor.Get(isolate);
    }
}