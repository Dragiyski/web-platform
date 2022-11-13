#include "function-template.h"

#include <cassert>
#include <map>
#include "../js-helper.h"

#include <functional>
#include "private.h"
#include "object-template.h"

namespace dragiyski::node_ext {
    DECLARE_API_WRAPPER_BODY_MORE(FunctionTemplate, initialize_more, uninitialize_more);

    namespace {
        std::map<v8::Isolate*, v8::Global<v8::FunctionTemplate>> per_isolate_default_function;
    }

    v8::Maybe<void> FunctionTemplate::initialize_more(v8::Isolate *isolate) {
        auto def_func = v8::FunctionTemplate::New(isolate, default_function);
        per_isolate_default_function.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, def_func)
        );
        return v8::JustVoid();
    }

    void FunctionTemplate::uninitialize_more(v8::Isolate *isolate) {
        per_isolate_default_function.erase(isolate);
    }

    v8::Maybe<void> FunctionTemplate::initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template) {
        class_template->Inherit(Template::get_template(isolate));
        auto signature = v8::Signature::New(isolate, class_template);
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "open");
            // Static method receiver does not use "this";
            auto value = v8::FunctionTemplate::New(isolate, static_open, v8::Local<v8::Value>(), v8::Local<v8::Signature>(), 1, v8::ConstructorBehavior::kThrow);
            value->SetClassName(name);
            class_template->Set(name, value);
        }
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "create");
            auto value = v8::FunctionTemplate::New(isolate, create, v8::Local<v8::Value>(), signature, 0, v8::ConstructorBehavior::kThrow);
            value->SetClassName(name);
            class_template->PrototypeTemplate()->Set(name, value);
        }
        return v8::JustVoid();
    }

    void FunctionTemplate::constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.This()->FindInstanceInPrototypeChain(get_template(isolate));
        if (
            !info.IsConstructCall() ||
            holder.IsEmpty() ||
            !holder->IsObject() ||
            holder.As<v8::Object>()->InternalFieldCount() < 1
            ) {
            JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal constructor");
        }

        v8::Local<v8::Function> callee;
        v8::Local<v8::String> name;
        v8::Local<v8::Object> options;
        v8::Local<v8::Private> cache_symbol;
        int length = 0;
        auto constructor_behavior = v8::ConstructorBehavior::kAllow;
        auto has_side_effects = v8::SideEffectType::kHasSideEffect;

        if (info.Length() >= 1) {
            if (info[0]->IsFunction()) {
                callee = info[0].As<v8::Function>();
                if (info.Length() >= 2 && info[1]->IsObject()) {
                    options = info[1].As<v8::Object>();
                }
            } else if (info[0]->IsObject()) {
                options = info[0].As<v8::Object>();
            }
        }

        if (!options.IsEmpty()) {
            if (callee.IsEmpty()) {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "function");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsFunction()) {
                        JS_THROW_ERROR(NOTHING, isolate, TypeError, "option[function]: not a function");
                    }
                    callee = js_value.As<v8::Function>();
                }
            }
            if (name.IsEmpty()) {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "name");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsString()) {
                        JS_THROW_ERROR(NOTHING, isolate, TypeError, "option[function]: not a function");
                    }
                    name = js_value.As<v8::String>();
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "length");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
                    length = value;
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "constructor");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->BooleanValue(isolate)) {
                        constructor_behavior = v8::ConstructorBehavior::kThrow;
                    }
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "sideEffect");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
                    if (value == 0) {
                        has_side_effects = v8::SideEffectType::kHasNoSideEffect;
                    } else if (value == 1) {
                        has_side_effects = v8::SideEffectType::kHasSideEffect;
                    } else if (value == 2) {
                        has_side_effects = v8::SideEffectType::kHasSideEffectToReceiver;
                    }
                }
            }
            {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "cache");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsObject()) {
                        JS_THROW_ERROR(NOTHING, isolate, TypeError, "option[cache]: not an object");
                    }
                    auto js_object = js_value.As<v8::Object>();
                    auto holder = js_object->FindInstanceInPrototypeChain(Private::get_template(isolate));
                    if (holder.IsEmpty()) {
                        JS_THROW_ERROR(NOTHING, isolate, TypeError, "option[cache]: not a 'Private' instance");
                    }
                    auto wrapper = node::ObjectWrap::Unwrap<Private>(holder);
                    cache_symbol = wrapper->value(isolate);
                }
            }
        }

        if (name.IsEmpty() && !callee.IsEmpty()) {
            auto callee_name = callee->GetName();
            if (callee_name->IsString()) {
                name = callee_name.As<v8::String>();
            }
        }

        auto api_callee = callee.IsEmpty() ? default_function : invoke;
        auto api_template = cache_symbol.IsEmpty() ?
            v8::FunctionTemplate::New(
                isolate,
                api_callee,
                holder,
                v8::Local<v8::Signature>(),
                length,
                constructor_behavior,
                has_side_effects
            ) :
            v8::FunctionTemplate::NewWithCache(
                isolate,
                api_callee,
                cache_symbol,
                holder,
                v8::Local<v8::Signature>(),
                length,
                has_side_effects
            );
        if (!name.IsEmpty()) {
            api_template->SetClassName(name);
        }

        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "instanceTemplate");
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, value, ObjectTemplate::New(context, api_template->InstanceTemplate()));
            JS_EXECUTE_IGNORE(NOTHING, info.This()->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT));
        }
        {
            auto name = v8::String::NewFromUtf8Literal(isolate, "prototypeTemplate");
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, value, ObjectTemplate::New(context, api_template->PrototypeTemplate()));
            JS_EXECUTE_IGNORE(NOTHING, info.This()->DefineOwnProperty(context, name, value, JS_PROPERTY_ATTRIBUTE_CONSTANT));
        }

        auto wrapper = new FunctionTemplate(isolate, api_template, callee);
        wrapper->Wrap(holder);

        info.GetReturnValue().Set(info.This());
    }

    FunctionTemplate::FunctionTemplate(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> value, v8::Local<v8::Function> callee) :
        _value(isolate, value),
        _callee(isolate, callee) {}

    v8::Local<v8::FunctionTemplate> FunctionTemplate::value(v8::Isolate *isolate) {
        return _value.Get(isolate);
    }

    v8::Local<v8::Function> FunctionTemplate::callee(v8::Isolate *isolate) {
        return _callee.Get(isolate);
    }

    v8::Maybe<FunctionTemplate *> FunctionTemplate::unwrap(v8::Isolate *isolate, v8::Local<v8::Object> object) {
        auto holder = object->FindInstanceInPrototypeChain(get_template(isolate));
        if (holder.IsEmpty()) {
            JS_THROW_ERROR(CPP_NOTHING(FunctionTemplate *), isolate, TypeError, "Cannot convert to 'FunctionTemplate'");
        }
        auto wrapper = node::ObjectWrap::Unwrap<FunctionTemplate>(holder);
        if (wrapper == nullptr) {
            JS_THROW_ERROR(CPP_NOTHING(FunctionTemplate *), isolate, TypeError, "Cannot convert to 'FunctionTemplate'");
        }
        return v8::Just(wrapper);
    }

    void FunctionTemplate::invoke(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Data().As<v8::Object>();

        // auto holder = this_arg->FindInstanceInPrototypeChain(get_template(isolate));
        // if (holder.IsEmpty() || !holder->IsObject() || holder.As<v8::Object>()->InternalFieldCount() < 1) {
        //     JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal invocation");
        // }

        JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, holder));
        auto callee = wrapper->callee(isolate);
        assert(!callee.IsEmpty());

        v8::Local<v8::Value> args_list[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            args_list[i] = info[i];
        }
        auto args_array = v8::Array::New(isolate, args_list, info.Length());

        v8::Local<v8::Value> args[] = { info.This(), args_array, info.NewTarget() };
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, return_value, callee->Call(context, holder, 3, args));
    }

    void FunctionTemplate::static_open(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        info.GetReturnValue().SetUndefined();
        if (info[0]->IsFunction()) {
            auto fn = info[0].As<v8::Function>();
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, open_value, fn->GetPrivate(context, get_private(isolate)));
            info.GetReturnValue().Set(open_value);
        }
    }

    void FunctionTemplate::create(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, info.Holder()));
        auto value = wrapper->value(isolate);

        auto callee_context = context;
        // TODO: If arguments.length >= 1 and arguments[0] instanceof Context (v8::FunctionTemplate), unwrap it to get v8::Local<v8::Context>
        // Then create a functon within that context instead of the current context.


        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, callee, value->GetFunction(callee_context));
        // Save the wrapper object reference, as template objects are neither accessible, nor comparable, so they cannot be stored in a map.
        // Thus for any object (including functions) we create, we assign the private to the created object.
        // Note: info.This() bacause it can be user defined extension (through class X extends FunctionTemplate).
        JS_EXECUTE_IGNORE(NOTHING, callee->SetPrivate(callee_context, get_private(isolate), info.This()));
        info.GetReturnValue().Set(callee);
    }

    void FunctionTemplate::default_function(const v8::FunctionCallbackInfo<v8::Value> &info) {
        if (info.IsConstructCall()) {
            info.GetReturnValue().Set(info.This());
        } else {
            info.GetReturnValue().SetUndefined();
        }
    }
}