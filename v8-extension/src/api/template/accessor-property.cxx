#include "accessor-property.hxx"
#include "../function-template.hxx"

#include <cassert>
#include <map>

#include "../../error-message.hxx"
#include "../../js-string-table.hxx"
#include "../context.hxx"

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_class_template;
        std::map<v8::Isolate *, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void Template::AccessorProperty::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_class_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = StringTable::Get(isolate, "AccessorProperty");
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
        class_template->SetClassName(class_name);
        // auto prototype_template = class_template->PrototypeTemplate();

        // Makes prototype *property* (not object) immutable similar to class X {}; syntax;
        class_template->ReadOnlyPrototype();

        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_class_symbol.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_cache)
        );

        per_isolate_class_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void Template::AccessorProperty::uninitialize(v8::Isolate *isolate) {
        per_isolate_class_template.erase(isolate);
        per_isolate_class_symbol.erase(isolate);
    }

    void Template::AccessorProperty::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }
        info.GetReturnValue().Set(info.This());

        auto class_template = AccessorProperty::get_class_template(isolate);

        auto holder = info.This()->FindInstanceInPrototypeChain(class_template);
        if (holder.IsEmpty() || !holder->IsObject() || holder->InternalFieldCount() < 1) {
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

        v8::Local<v8::Object> getter_object;
        {
            auto name = StringTable::Get(isolate, "getter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Required option \"getter\": not an object.");
                }
                getter_object = value.As<v8::Object>();
            }
        }
        auto function_template_template = FunctionTemplate::get_class_template(isolate);
        v8::Local<v8::FunctionTemplate> getter_template;
        if (!getter_object.IsEmpty()) {
            JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(
                wrapper,
                Wrapper::Unwrap<FunctionTemplate>(
                    isolate,
                    getter_object,
                    function_template_template,
                    "FunctionTemplate"
                ),
                context,
                "Option \"getter\""
            );
            getter_template = wrapper->get_value(isolate);
        }

        v8::Local<v8::Object> setter_object;
        {
            auto name = StringTable::Get(isolate, "setter");
            JS_EXPRESSION_RETURN(value, options->Get(context, name));
            if (!value->IsNullOrUndefined()) {
                if (!value->IsObject()) {
                    JS_THROW_ERROR(TypeError, isolate, "Required option \"setter\": not an object.");
                }
                setter_object = value.As<v8::Object>();
            }
        }
        v8::Local<v8::FunctionTemplate> setter_template;
        if (!setter_object.IsEmpty()) {
            JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(
                wrapper,
                Wrapper::Unwrap<FunctionTemplate>(
                    isolate,
                    setter_object,
                    function_template_template,
                    "FunctionTemplate"
                ),
                context,
                "Option \"setter\""
            );
            setter_template = wrapper->get_value(isolate);
        }

        auto attributes = JS_PROPERTY_ATTRIBUTE_DEFAULT;
        {
            auto name = StringTable::Get(isolate, "attributes");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"attributes\"");
                value = value & static_cast<uint32_t>(JS_PROPERTY_ATTRIBUTE_ALL);
                attributes = static_cast<v8::PropertyAttribute>(value);
            }
        }

        auto access_control = v8::AccessControl::DEFAULT;
        {
            auto name = StringTable::Get(isolate, "accessControl");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN_WITH_ERROR_PREFIX(value, js_value->Uint32Value(context), context, "In option \"accessControl\"");
                if (value != v8::AccessControl::DEFAULT && value != v8::AccessControl::ALL_CAN_READ && value != v8::AccessControl::ALL_CAN_WRITE) {
                    JS_THROW_ERROR(TypeError, isolate, "Option \"accessControl\": not a valid access control value.");
                }
                access_control = static_cast<v8::AccessControl>(value);
            }
        }

        JS_EXPRESSION_IGNORE(holder->SetPrivate(context, Wrapper::get_this_symbol(isolate), info.This()));

        auto wrapper = new AccessorProperty(isolate, getter_object, getter_template, setter_object, setter_template, attributes, access_control);
        wrapper->Wrap(isolate, holder);
    }

    v8::Local<v8::FunctionTemplate> Template::AccessorProperty::get_getter(v8::Isolate *isolate) const {
        return _getter.Get(isolate);
    }

    v8::Local<v8::Object> Template::AccessorProperty::get_getter_object(v8::Isolate *isolate) const {
        return _getter_object.Get(isolate);
    }

    v8::Local<v8::FunctionTemplate> Template::AccessorProperty::get_setter(v8::Isolate *isolate) const {
        return _setter.Get(isolate);
    }

    v8::Local<v8::Object> Template::AccessorProperty::get_setter_object(v8::Isolate *isolate) const {
        return _setter_object.Get(isolate);
    }

    v8::PropertyAttribute Template::AccessorProperty::get_attributes() const {
        return _attributes;
    }

    v8::AccessControl Template::AccessorProperty::get_access_control() const {
        return _access_control;
    }

    v8::Maybe<void> Template::AccessorProperty::setup(v8::Isolate *isolate, v8::Local<v8::Template> target, v8::Local<v8::Name> name, v8::Local<v8::Object> js_template_wrapper) const {
        static constexpr const auto __function_return_type__ = v8::Nothing<void>;
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        JS_EXPRESSION_RETURN(control_context, js_template_wrapper->GetCreationContext());
        if V8_UNLIKELY (control_context.IsEmpty()) {
            control_context = context;
        }

        auto this_symbol = Wrapper::get_this_symbol(isolate);

        JS_EXPRESSION_RETURN(js_context_holder, Context::get_context_holder(control_context, context));
        JS_EXPRESSION_RETURN(js_context_wrapper, js_context_holder->GetPrivate(control_context, this_symbol));

        auto this_holder = get_holder(isolate);
        JS_EXPRESSION_RETURN(this_self, this_holder->GetPrivate(context, this_symbol));

        target->SetAccessorProperty(
            name,
            get_getter(isolate),
            get_setter(isolate),
            get_attributes(),
            get_access_control()
        );
        return v8::JustVoid();
    }

    Template::AccessorProperty::AccessorProperty(
        v8::Isolate* isolate,
        v8::Local<v8::Object> getter_object,
        v8::Local<v8::FunctionTemplate> getter,
        v8::Local<v8::Object> setter_object,
        v8::Local<v8::FunctionTemplate> setter,
        v8::PropertyAttribute attributes,
        v8::AccessControl access_control
    ) :
        _getter_object(isolate, getter_object),
        _getter(isolate, getter),
        _setter_object(isolate, setter_object),
        _setter(isolate, setter),
        _attributes(attributes),
        _access_control(access_control) {}
}