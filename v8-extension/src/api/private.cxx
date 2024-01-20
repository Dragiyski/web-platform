#include "private.hxx"

#include "../js-string-table.hxx"
#include <cassert>
#include <map>

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_template;
    }

    void Private::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_template.contains(isolate));

        auto class_name = StringTable::Get(isolate, "Private");
        auto class_template = v8::FunctionTemplate::New(isolate, constructor);
        class_template->SetClassName(class_name);
        auto prototype_template = class_template->PrototypeTemplate();
        auto signature = v8::Signature::New(isolate, class_template);
        {
            auto name = StringTable::Get(isolate, "get");
            auto value = v8::FunctionTemplate::New(
                isolate,
                prototype_get,
                {},
                signature,
                1,
                v8::ConstructorBehavior::kThrow
            );
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "set");
            auto value = v8::FunctionTemplate::New(
                isolate,
                prototype_set,
                {},
                signature,
                2,
                v8::ConstructorBehavior::kThrow
            );
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "has");
            auto value = v8::FunctionTemplate::New(
                isolate,
                prototype_has,
                {},
                signature,
                1,
                v8::ConstructorBehavior::kThrow
            );
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "delete");
            auto value = v8::FunctionTemplate::New(
                isolate,
                prototype_delete,
                {},
                signature,
                1,
                v8::ConstructorBehavior::kThrow
            );
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }

        // Makes prototype *property* (not object) immutable similar to class X {}; syntax;
        class_template->ReadOnlyPrototype();
        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );

        Object<Private>::initialize(isolate);
    }

    void Private::uninitialize(v8::Isolate *isolate) {
        Object<Private>::uninitialize(isolate);
        per_isolate_template.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> Private::get_template(v8::Isolate *isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    void Private::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        
        if V8_UNLIKELY(!info.IsConstructCall()) {
            JS_THROW_ERROR(TypeError, isolate, "Class constructor ", "Private", " cannot be invoked without 'new'");
        }

        v8::Local<v8::String> name;
        if (!info[0]->IsNullOrUndefined()) {
            if V8_UNLIKELY(!info[0]->IsString()) {
                JS_THROW_ERROR(TypeError, isolate, "Expected arguments[0] to be a string, if specified.");
            }
            name = info[0].As<v8::String>();
        }

        auto value = v8::Private::New(isolate, name);
        auto implementation = new Private(isolate, value);
        implementation->set_interface(isolate, info.This());
        info.GetReturnValue().Set(info.This());
    };

    void Private::prototype_get(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (!info.IsConstructCall()) {
            JS_THROW_ERROR(TypeError, isolate, "Class constructor ", "Private", " cannot be invoked without 'new'");
        }

        auto implementation = get_implementation(isolate, info.This());
        if (implementation == nullptr) {
            v8::Local<v8::String> receiver_type;
            JS_EXPRESSION_RETURN(receiver, type_of(context, info.This()));
            JS_THROW_ERROR(TypeError, isolate, "Incompatible receiver ", receiver);
        }

        if V8_UNLIKELY(info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, context, "Expected arguments[0] to be an [object], got ", type_of(context, info[0]));
        }
        auto object = info[0].As<v8::Object>();
        auto value = implementation->get_value(isolate);

        JS_EXPRESSION_RETURN(has_private, object->HasPrivate(context, value));
        if V8_UNLIKELY(!has_private) {
            info.GetReturnValue().SetUndefined();
            return;
        }
        JS_EXPRESSION_RETURN(return_value, object->GetPrivate(context, value));
        info.GetReturnValue().Set(return_value);
    }

    void Private::prototype_set(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto implementation = get_implementation(isolate, info.This());
        if (implementation == nullptr) {
            JS_EXPRESSION_RETURN(receiver, type_of(context, info.This()));
            JS_THROW_ERROR(TypeError, isolate, "Private", ".", "prototype", ".", "set", " called on incompatible receiver ", receiver);
        }

        if V8_UNLIKELY(info.Length() < 2) {
            JS_THROW_ERROR(TypeError, isolate, "2 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "Expected arguments[0] to be an object.");
        }
        auto object = info[0].As<v8::Object>();
        auto value = implementation->get_value(isolate);

        JS_EXPRESSION_RETURN(has_private, object->HasPrivate(context, value));
        if V8_UNLIKELY(!has_private) {
            info.GetReturnValue().SetUndefined();
        } else {
            JS_EXPRESSION_RETURN(return_value, object->GetPrivate(context, value));
            info.GetReturnValue().Set(return_value);
        }

        JS_EXPRESSION_IGNORE(object->SetPrivate(context, value, info[1]));
    }

    void Private::prototype_has(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto implementation = get_implementation(isolate, info.This());
        if (implementation == nullptr) {
            JS_EXPRESSION_RETURN(receiver, type_of(context, info.This()));
            JS_THROW_ERROR(TypeError, isolate, "Private", ".", "prototype", ".", "has", " called on incompatible receiver ", receiver);
        }

        if V8_UNLIKELY(info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "Expected arguments[0] to be an object.");
        }
        auto object = info[0].As<v8::Object>();
        auto value = implementation->get_value(isolate);

        JS_EXPRESSION_RETURN(return_value, object->HasPrivate(context, value));
        info.GetReturnValue().Set(return_value);
    }

    void Private::prototype_delete(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto implementation = get_implementation(isolate, info.This());
        if (implementation == nullptr) {
            JS_EXPRESSION_RETURN(receiver, type_of(context, info.This()));
            JS_THROW_ERROR(TypeError, isolate, "Private", ".", "prototype", ".", "delete", " called on incompatible receiver ", receiver);
        }

        if V8_UNLIKELY(info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "Expected arguments[0] to be an object.");
        }
        auto object = info[0].As<v8::Object>();
        auto value = implementation->get_value(isolate);

        JS_EXPRESSION_RETURN(has_private, object->HasPrivate(context, value));
        if V8_UNLIKELY(!has_private) {
            info.GetReturnValue().Set(false);
            return;
        }
        JS_EXPRESSION_RETURN(return_value, object->DeletePrivate(context, value));
        info.GetReturnValue().Set(return_value);
    }

    Private::Private(v8::Isolate *isolate, v8::Local<v8::Private> value) :
        _value(isolate, value) {}

    v8::Local<v8::Private> Private::get_value(v8::Isolate *isolate) const {
        return _value.Get(isolate);
    }
}
