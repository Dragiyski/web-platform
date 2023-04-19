#include "private.hxx"

#include "../js-string-table.hxx"
#include <cassert>
#include <map>

namespace dragiyski::node_ext {
    namespace {
        std::map<v8::Isolate *, Shared<v8::FunctionTemplate>> per_isolate_class_template;
        std::map<v8::Isolate *, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void Private::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_class_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = StringTable::Get(isolate, "Private");
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
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

    void Private::uninitialize(v8::Isolate *isolate) {
        per_isolate_class_template.erase(isolate);
        per_isolate_class_symbol.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> Private::get_class_template(v8::Isolate *isolate) {
        assert(per_isolate_class_template.contains(isolate));
        return per_isolate_class_template[isolate].Get(isolate);
    }

    v8::Local<v8::Private> Private::get_class_symbol(v8::Isolate *isolate) {
        assert(per_isolate_class_symbol.contains(isolate));
        return per_isolate_class_symbol[isolate].Get(isolate);
    }

    void Private::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if V8_UNLIKELY(!info.IsConstructCall()) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }

        auto self = info.This();
        auto self_template = get_class_template(isolate);

        info.GetReturnValue().Set(self);
        auto holder = info.This()->FindInstanceInPrototypeChain(self_template);
        if V8_UNLIKELY(holder.IsEmpty() || !holder->IsObject() || holder->InternalFieldCount() < 1) {
            auto message = StringTable::Get(isolate, "Illegal constructor");
            JS_THROW_ERROR(TypeError, isolate, message);
        }

        v8::Local<v8::String> name;
        if (!info[0]->IsNullOrUndefined()) {
            if V8_UNLIKELY(!info[0]->IsString()) {
                JS_THROW_ERROR(TypeError, isolate, "Expected arguments[0] to be a string.");
            }
            name = info[0].As<v8::String>();
        }

        JS_EXPRESSION_IGNORE(holder->SetPrivate(context, Wrapper::get_this_symbol(isolate), info.This()));

        auto value = v8::Private::New(isolate, name);
        auto wrapper = new Private(isolate, value);
        wrapper->Wrap(isolate, holder);
    };

    void Private::prototype_get(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if V8_UNLIKELY(info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        }
        auto object = info[0].As<v8::Object>();

        auto holder = info.Holder();
        auto wrapper = Unwrap<Private>(isolate, holder);
        auto value = wrapper->get_value(isolate);

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

        if V8_UNLIKELY(info.Length() < 2) {
            JS_THROW_ERROR(TypeError, isolate, "2 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        }
        auto object = info[0].As<v8::Object>();

        auto holder = info.Holder();
        auto wrapper = Unwrap<Private>(isolate, holder);
        auto value = wrapper->get_value(isolate);

        JS_EXPRESSION_RETURN(return_value, object->SetPrivate(context, value, info[1]));
        info.GetReturnValue().Set(return_value);
    }

    void Private::prototype_has(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if V8_UNLIKELY(info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        }
        auto object = info[0].As<v8::Object>();

        auto holder = info.Holder();
        auto wrapper = Unwrap<Private>(isolate, holder);
        auto value = wrapper->get_value(isolate);

        JS_EXPRESSION_RETURN(return_value, object->HasPrivate(context, value));
        info.GetReturnValue().Set(return_value);
    }

    void Private::prototype_delete(const v8::FunctionCallbackInfo<v8::Value> &info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if V8_UNLIKELY(info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }
        if V8_UNLIKELY(!info[0]->IsObject()) {
            JS_THROW_ERROR(TypeError, isolate, "argument 1 is not not an object.");
        }
        auto object = info[0].As<v8::Object>();

        auto holder = info.Holder();
        auto wrapper = Unwrap<Private>(isolate, holder);
        auto value = wrapper->get_value(isolate);

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
