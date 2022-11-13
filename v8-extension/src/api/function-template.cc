#include "function-template.h"

#include <cassert>
#include <map>
#include "../js-helper.h"

#include <functional>
#include "private.h"
#include "object-template.h"

namespace dragiyski::node_ext {
    DECLARE_API_WRAPPER_BODY(FunctionTemplate);

    v8::Maybe<void> FunctionTemplate::initialize_template(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> class_template) {
        return v8::JustVoid();
    }

    void FunctionTemplate::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
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
            JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
        }


        v8::Local<v8::Function> callee;
        v8::Local<v8::String> name;
        v8::Local<v8::Object> options;
        v8::Local<v8::Private> cache_symbol;
        int length = 0;
        auto constructor_behavior = v8::ConstructorBehavior::kAllow;
        auto has_side_effects = v8::SideEffectType::kHasSideEffect;

        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (info[0]->IsFunction()) {
            callee = info[0].As<v8::Function>();
            if (info.Length() >= 2 && info[1]->IsObject()) {
                options = info[1].As<v8::Object>();
            }
        } else if (info[0]->IsObject()) {
            options = info[0].As<v8::Object>();
        }

        if (!options.IsEmpty()) {
            if (callee.IsEmpty()) {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "function");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsFunction()) {
                        JS_THROW_ERROR(NOTHING, context, TypeError, "option[function]: not a function");
                    }
                    callee = js_value.As<v8::Function>();
                }
            }
            if (name.IsEmpty()) {
                JS_OBJECT_GET_KEY_HANDLE(NOTHING, v8::Value, js_value, context, options, "name");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsString()) {
                        JS_THROW_ERROR(NOTHING, context, TypeError, "option[function]: not a function");
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
                        JS_THROW_ERROR(NOTHING, context, TypeError, "option[cache]: not an object");
                    }
                    auto js_object = js_value.As<v8::Object>();
                    auto holder = js_object->FindInstanceInPrototypeChain(Private::get_template(isolate));
                    if (holder.IsEmpty()) {
                        JS_THROW_ERROR(NOTHING, context, TypeError, "option[cache]: not a 'Private' instance");
                    }
                    auto wrapper = node::ObjectWrap::Unwrap<Private>(holder);
                    cache_symbol = wrapper->value(isolate);
                }
            }
        }

        auto api_callee = callee.IsEmpty() ? nullptr : FunctionTemplate::invoke;
        auto api_template = cache_symbol.IsEmpty() ?
            v8::FunctionTemplate::New(
                isolate,
                api_callee,
                callee,
                v8::Local<v8::Signature>(),
                length,
                constructor_behavior,
                has_side_effects
            ) :
            v8::FunctionTemplate::NewWithCache(
                isolate,
                api_callee,
                cache_symbol,
                callee,
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

        auto wrapper = new FunctionTemplate(isolate, api_template);
        wrapper->Wrap(holder);

        info.GetReturnValue().Set(info.This());
    }

    FunctionTemplate::FunctionTemplate(v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> value) : _value(isolate, value) {
    }

    v8::Local<v8::FunctionTemplate> FunctionTemplate::value(v8::Isolate *isolate) {
        return _value.Get(isolate);
    }

    void FunctionTemplate::invoke(const v8::FunctionCallbackInfo<v8::Value>& info) {
        info.GetReturnValue().SetNull();
    }
}