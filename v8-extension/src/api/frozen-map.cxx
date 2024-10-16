#include "frozen-map.hxx"

#include "../js-string-table.hxx"
#include "../error-message.hxx"
#include <map>

namespace dragiyski::node_ext {
    using namespace js;
    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_template;
        std::map<v8::Isolate*, Shared<v8::ObjectTemplate>> per_isolate_iterator_template;
    }

    void FrozenMap::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_template.contains(isolate));

        auto class_name = ::js::StringTable::Get(isolate, "FrozenMap");
        auto class_template = v8::FunctionTemplate::New(isolate, constructor, {}, {}, 1);
        class_template->SetClassName(class_name);

        auto signature = v8::Signature::New(isolate, class_template);
        auto prototype_template = class_template->PrototypeTemplate();
        {
            auto name = StringTable::Get(isolate, "get");
            auto value = v8::FunctionTemplate::New(isolate, prototype_get, {}, signature, 0, v8::ConstructorBehavior::kThrow);
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "has");
            auto value = v8::FunctionTemplate::New(isolate, prototype_has, {}, signature, 0, v8::ConstructorBehavior::kThrow);
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "size");
            auto value = v8::FunctionTemplate::New(isolate, prototype_size, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->SetAccessorProperty(name, value, {}, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto entries = v8::FunctionTemplate::New(isolate, prototype_entries, {}, signature, 0, v8::ConstructorBehavior::kThrow);
            {
                auto name = StringTable::Get(isolate, "entries");
                prototype_template->Set(name, entries, JS_PROPERTY_ATTRIBUTE_STATIC);
            }
            {
                auto name = v8::Symbol::GetIterator(isolate);
                prototype_template->Set(name, entries, JS_PROPERTY_ATTRIBUTE_STATIC);
            }
        }
        {
            auto name = StringTable::Get(isolate, "keys");
            auto value = v8::FunctionTemplate::New(isolate, prototype_keys, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }
        {
            auto name = StringTable::Get(isolate, "values");
            auto value = v8::FunctionTemplate::New(isolate, prototype_values, {}, signature, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            prototype_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }

        class_template->ReadOnlyPrototype();
        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );

        Object<FrozenMap>::initialize(isolate);
        Iterator::initialize(isolate);
    }

    void FrozenMap::uninitialize(v8::Isolate* isolate) {
        Iterator::uninitialize(isolate);
        Object<FrozenMap>::uninitialize(isolate);
        per_isolate_template.erase(isolate);
    }

    void FrozenMap::Iterator::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_iterator_template.contains(isolate));

        auto class_name = StringTable::Get(isolate, "FrozenMap Iterator");
        auto iterator_template = v8::ObjectTemplate::New(isolate);

        iterator_template->SetInternalFieldCount(1);

        {
            auto name = StringTable::Get(isolate, "next");
            auto value = v8::FunctionTemplate::New(isolate, prototype_next, {}, {}, 0, v8::ConstructorBehavior::kThrow, v8::SideEffectType::kHasNoSideEffect);
            iterator_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_STATIC);
        }

        {
            auto name = v8::Symbol::GetToStringTag(isolate);
            iterator_template->Set(name, class_name, JS_PROPERTY_ATTRIBUTE_STATIC);
        }

        per_isolate_iterator_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, iterator_template)
        );

        Object<FrozenMap::Iterator>::initialize(isolate);
    }

    void FrozenMap::Iterator::uninitialize(v8::Isolate* isolate) {
        Object<FrozenMap::Iterator>::uninitialize(isolate);
        per_isolate_template.erase(isolate);
    }

    v8::Local<v8::ObjectTemplate> FrozenMap::Iterator::get_template(v8::Isolate* isolate) {
        assert(per_isolate_iterator_template.contains(isolate));
        return per_isolate_iterator_template[isolate].Get(isolate);
    }

    void FrozenMap::Iterator::prototype_next(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        {
            auto holder = info.This();
            if V8_UNLIKELY(!holder->IsObject()) {
                goto invalid_this;
            }
            auto iterator = Object<FrozenMap::Iterator>::get_implementation(isolate, holder);
            if (iterator == nullptr) {
                goto invalid_this;
            }
            auto key_value = iterator->_key_value.Get(isolate);
            auto key_value_size = key_value->Length();
            auto iteration = v8::Object::New(isolate);
            auto string_done = StringTable::Get(isolate, "done");
            auto string_value = StringTable::Get(isolate, "value");
            if (iterator->_index < key_value_size) {
                v8::Local<v8::Value> iteration_value;
                if (iterator->_with_key && iterator->_with_value) {
                    JS_EXPRESSION_RETURN(key, key_value->Get(context, iterator->_index));
                    JS_EXPRESSION_RETURN(value, key_value->Get(context, iterator->_index + 1));
                    v8::Local<v8::Value> elements[] = { key, value };
                    iteration_value = v8::Array::New(isolate, elements, 2);
                } else if (iterator->_with_key) {
                    JS_EXPRESSION_RETURN(key, key_value->Get(context, iterator->_index));
                    iteration_value = key;
                } else if (iterator->_with_value) {
                    JS_EXPRESSION_RETURN(value, key_value->Get(context, iterator->_index + 1));
                    iteration_value = value;
                } else {
                    goto invalid_this;
                }
                JS_EXPRESSION_IGNORE(iteration->Set(context, string_value, iteration_value));
                JS_EXPRESSION_IGNORE(iteration->Set(context, string_done, v8::False(isolate)));
                iterator->_index += 2;
            } else {
                JS_EXPRESSION_IGNORE(iteration->Set(context, string_value, v8::Undefined(isolate)));
                JS_EXPRESSION_IGNORE(iteration->Set(context, string_done, v8::True(isolate)));
            }
            info.GetReturnValue().Set(iteration);
        }

        invalid_this:
        {
            JS_EXPRESSION_RETURN(receiver, type_of(context, info.This()));
            JS_THROW_ERROR(TypeError, isolate, "Method ", "FrozenMap Iterator", ".", "next", " called on incompatible receiver ", receiver);
        }
    }

    v8::Local<v8::FunctionTemplate> FrozenMap::get_template(v8::Isolate* isolate) {
        assert(per_isolate_template.contains(isolate));
        return per_isolate_template[isolate].Get(isolate);
    }

    void FrozenMap::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        if (!info.IsConstructCall()) {
            JS_THROW_ERROR(TypeError, isolate, "Class constructor ", "FrozenMap", " cannot be invoked without 'new'");
        }

        if (info.This()->InternalFieldCount() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal constructor");
        }

        if (info.Length() < 1) {
            JS_THROW_ERROR(TypeError, isolate, "1 argument required, but only ", info.Length(), " present.");
        }

        v8::Local<v8::Map> source;
        if V8_LIKELY(info[0]->IsMap()) {
            source = info[0].As<v8::Map>();
        } else if (info[0]->IsObject()) {
            auto frozen_map = Object<FrozenMap>::get_implementation(isolate, info[0].As<v8::Object>());
            if V8_LIKELY(frozen_map != nullptr) {
                source = frozen_map->_map.Get(isolate);
            }
        }
        if V8_UNLIKELY(source.IsEmpty()) {
            JS_THROW_ERROR(TypeError, isolate, "Argument 1 is not an [object Map] or [object FrozenMap]");
        }

        auto implementation = new FrozenMap();
        implementation->_map.Reset(isolate, source);
        implementation->set_interface(isolate, info.This());

        info.GetReturnValue().Set(info.This());
    }

    void FrozenMap::prototype_get(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        auto implementation = Object<FrozenMap>::get_own_implementation(isolate, holder);
        if (implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto map = implementation->_map.Get(isolate);
        JS_EXPRESSION_RETURN(value, map->Get(context, info[0]));
        info.GetReturnValue().Set(value);
    }

    void FrozenMap::prototype_has(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        auto implementation = Object<FrozenMap>::get_own_implementation(isolate, holder);
        if (implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto map = implementation->_map.Get(isolate);
        JS_EXPRESSION_RETURN(value, map->Has(context, info[0]));
        info.GetReturnValue().Set(value);
    }

    void FrozenMap::prototype_size(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        auto holder = info.Holder();
        auto implementation = Object<FrozenMap>::get_own_implementation(isolate, holder);
        if (implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto map = implementation->_map.Get(isolate);
        auto size = map->Size();
        info.GetReturnValue().Set(static_cast<uint32_t>(size));
    }

    void FrozenMap::prototype_entries(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        auto implementation = Object<FrozenMap>::get_own_implementation(isolate, holder);
        if (implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto map = implementation->_map.Get(isolate);

        auto iterator_template = FrozenMap::Iterator::get_template(isolate);
        JS_EXPRESSION_RETURN(iterator_object, iterator_template->NewInstance(context));
        auto iterator = new Iterator();
        auto key_value = map->AsArray();
        iterator->_key_value.Reset(isolate, key_value);
        iterator->_index = 0;
        iterator->_with_key = true;
        iterator->_with_value = true;
        iterator->set_interface(isolate, iterator_object);
        info.GetReturnValue().Set(iterator_object);
    }

    void FrozenMap::prototype_keys(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        auto implementation = Object<FrozenMap>::get_own_implementation(isolate, holder);
        if (implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto map = implementation->_map.Get(isolate);

        auto iterator_template = FrozenMap::Iterator::get_template(isolate);
        JS_EXPRESSION_RETURN(iterator_object, iterator_template->NewInstance(context));
        auto iterator = new Iterator();
        auto key_value = map->AsArray();
        iterator->_key_value.Reset(isolate, key_value);
        iterator->_index = 0;
        iterator->_with_key = true;
        iterator->_with_value = false;
        iterator->set_interface(isolate, iterator_object);
        info.GetReturnValue().Set(iterator_object);
    }

    void FrozenMap::prototype_values(const v8::FunctionCallbackInfo<v8::Value>& info) {
        using __function_return_type__ = void;
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Holder();
        auto implementation = Object<FrozenMap>::get_own_implementation(isolate, holder);
        if (implementation == nullptr) {
            JS_THROW_ERROR(TypeError, isolate, "Illegal invocation");
        }
        auto map = implementation->_map.Get(isolate);

        auto iterator_template = FrozenMap::Iterator::get_template(isolate);
        JS_EXPRESSION_RETURN(iterator_object, iterator_template->NewInstance(context));
        auto iterator = new Iterator();
        auto key_value = map->AsArray();
        iterator->_key_value.Reset(isolate, key_value);
        iterator->_index = 0;
        iterator->_with_key = false;
        iterator->_with_value = true;
        iterator->set_interface(isolate, iterator_object);
        info.GetReturnValue().Set(iterator_object);
    }

    v8::MaybeLocal<v8::Object> FrozenMap::Create(v8::Local<v8::Context> context, v8::Local<v8::Map> map) {
        using __function_return_type__ = v8::MaybeLocal<v8::Object>;
        auto isolate = context->GetIsolate();
        v8::EscapableHandleScope scope(isolate);
        
        auto class_template = FrozenMap::get_template(isolate);
        JS_EXPRESSION_RETURN(interface, class_template->InstanceTemplate()->NewInstance(context));
        auto implementation = new FrozenMap();
        implementation->_map.Reset(isolate, map);
        implementation->set_interface(isolate, interface);
        return scope.Escape(interface);
    }

    v8::Local<v8::Map> FrozenMap::get_map(v8::Isolate* isolate) {
        return _map.Get(isolate);
    }
}