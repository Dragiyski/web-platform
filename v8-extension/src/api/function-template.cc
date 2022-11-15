#include "function-template.h"

#include <cassert>
#include <map>
#include "../js-helper.h"

#include <functional>
#include "private.h"
#include "object-template.h"
#include "../string-table.h"

namespace dragiyski::node_ext {
    using namespace v8_handles;
    DECLARE_API_WRAPPER_BODY_MORE(FunctionTemplate, initialize_more, uninitialize_more);

    namespace {
        std::map<v8::Isolate *, v8::Global<v8::FunctionTemplate>> per_isolate_default_function;
        std::map<v8::Isolate *, std::map<const char *, v8::Global<v8::String>>> per_isolate_names;
    }

    Maybe<void> FunctionTemplate::initialize_more(v8::Isolate *isolate) {
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
        per_isolate_names.erase(isolate);
    }

    Maybe<void> FunctionTemplate::initialize_template(v8::Isolate *isolate, Local<v8::FunctionTemplate> class_template) {
        class_template->Inherit(Template::get_template(isolate));
        auto signature = v8::Signature::New(isolate, class_template);
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "open");
            auto value = v8::FunctionTemplate::New(isolate, static_open, {}, Local<v8::Signature>(), 1, v8::ConstructorBehavior::kThrow);
            value->SetClassName(name);
            class_template->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "create");
            auto value = v8::FunctionTemplate::New(isolate, create, {}, signature, 0, v8::ConstructorBehavior::kThrow);
            value->SetClassName(name);
            class_template->PrototypeTemplate()->Set(name, value, JS_PROPERTY_ATTRIBUTE_FROZEN);
        }
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "instanceTemplate");
            auto value = v8::FunctionTemplate::New(
                isolate,
                get_instance_template,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasNoSideEffect
            );
            class_template->PrototypeTemplate()->SetAccessorProperty(
                name,
                value,
                Local<v8::FunctionTemplate>(),
                JS_PROPERTY_ATTRIBUTE_FROZEN
            );
        }
        {
            JS_PROPERTY_NAME(VOID_NOTHING, name, isolate, "prototypeTemplate");
            auto value = v8::FunctionTemplate::New(
                isolate,
                get_prototype_template,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasNoSideEffect
            );
            class_template->PrototypeTemplate()->SetAccessorProperty(
                name,
                value,
                Local<v8::FunctionTemplate>(),
                JS_PROPERTY_ATTRIBUTE_FROZEN
            );
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
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, message, string_map::get_string(isolate, "Illegal constructor"));
            JS_THROW_ERROR(NOTHING, isolate, TypeError, message);
        }

        Local<v8::Function> callee;
        Local<v8::String> name;
        Local<v8::Object> options;
        Local<v8::Private> cache_symbol;
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
                JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "function");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsFunction()) {
                        JS_THROW_ERROR(NOTHING, isolate, TypeError, "option[function]: not a function");
                    }
                    callee = js_value.As<v8::Function>();
                }
            }
            if (name.IsEmpty()) {
                JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "name");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsString()) {
                        JS_THROW_ERROR(NOTHING, isolate, TypeError, "option[function]: not a function");
                    }
                    name = js_value.As<v8::String>();
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "length");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
                    length = value;
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "constructor");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->BooleanValue(isolate)) {
                        constructor_behavior = v8::ConstructorBehavior::kThrow;
                    }
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "sideEffect");
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
                JS_OBJECT_GET_LITERAL_KEY(NOTHING, js_value, context, options, "cache");
                if (!js_value->IsNullOrUndefined()) {
                    if (!js_value->IsObject()) {
                        JS_THROW_ERROR(NOTHING, isolate, TypeError, "option[cache]: not an object");
                    }
                    auto js_object = js_value.As<v8::Object>();
                    JS_EXECUTE_RETURN(NOTHING, Private *, wrapper, Private::unwrap(isolate, js_object));
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
                Local<v8::Signature>(),
                length,
                constructor_behavior,
                has_side_effects
            ) :
            v8::FunctionTemplate::NewWithCache(
                isolate,
                api_callee,
                cache_symbol,
                holder,
                Local<v8::Signature>(),
                length,
                has_side_effects
            );
        if (!name.IsEmpty()) {
            api_template->SetClassName(name);
        }

        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, instance_template, ObjectTemplate::New(context, api_template->InstanceTemplate()));
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Object, prototype_template, ObjectTemplate::New(context, api_template->PrototypeTemplate()));

        auto wrapper = new FunctionTemplate(
            isolate,
            api_template,
            instance_template,
            prototype_template,
            callee);
        wrapper->Wrap(holder, info.This());

        info.GetReturnValue().Set(info.This());
    }

    FunctionTemplate::FunctionTemplate(
        v8::Isolate *isolate,
        Local<v8::FunctionTemplate> api_template,
        Local<v8::Object> instance_template,
        Local<v8::Object> prototype_template,
        Local<v8::Function> callee) :
        Template(isolate),
        _template(isolate, api_template),
        _instance_template_holder(isolate, instance_template),
        _prototype_template_holder(isolate, prototype_template),
        _callee(isolate, callee) {}

    Local<v8::FunctionTemplate> FunctionTemplate::this_function_template(v8::Isolate *isolate) {
        return _template.Get(isolate);
    }

    Local<v8::Template> FunctionTemplate::this_template(v8::Isolate *isolate) {
        return _template.Get(isolate);
    }

    Local<v8::Object> FunctionTemplate::this_instance_template_holder(v8::Isolate *isolate) {
        return _instance_template_holder.Get(isolate);
    }

    Local<v8::Object> FunctionTemplate::this_prototype_template_holder(v8::Isolate *isolate) {
        return _prototype_template_holder.Get(isolate);
    }

    Local<v8::Function> FunctionTemplate::callee(v8::Isolate *isolate) {
        return _callee.Get(isolate);
    }

    void FunctionTemplate::invoke(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto holder = info.Data().As<v8::Object>();
        assert(!holder.IsEmpty() && holder->IsObject() && holder.As<v8::Object>()->InternalFieldCount() >= 1);

        JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, holder));
        auto callee = wrapper->callee(isolate);
        assert(!callee.IsEmpty());

        Local<v8::Value> args_list[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            args_list[i] = info[i];
        }
        auto args_array = v8::Array::New(isolate, args_list, info.Length());

        Local<v8::Value> args[] = { info.This(), args_array, info.NewTarget() };
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
        if (info[0]->IsObject()) {
            auto holder = info[0].As<v8::Object>();
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, open_value, holder->GetPrivate(context, get_private(isolate)));
            info.GetReturnValue().Set(open_value);
        }
    }

    void FunctionTemplate::create(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, info.Holder()));
        auto value = wrapper->this_function_template(isolate);

        auto callee_context = context;
        // TODO: If arguments.length >= 1 and arguments[0] instanceof Context (v8::FunctionTemplate), unwrap it to get Local<v8::Context>
        // Then create a functon within that context instead of the current context.


        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, callee, value->GetFunction(callee_context));
        // Save the wrapper object reference, as template objects are neither accessible, nor comparable, so they cannot be stored in a map.
        // Thus for any object (including functions) we create, we assign the private to the created object.
        // Note: info.This() bacause it can be user defined extension (through class X extends FunctionTemplate).
        JS_EXECUTE_IGNORE(NOTHING, callee->SetPrivate(callee_context, get_private(isolate), info.This()));
        info.GetReturnValue().Set(callee);
    }

    // For some reason, no-api function apply function that always return this.
    // In regular function (non-constructor) call, "this" will become the global object in non-strict mode,
    // which is unexpected for "no-function". We declare the following replacement instead.
    void FunctionTemplate::default_function(const v8::FunctionCallbackInfo<v8::Value> &info) {
        if (info.IsConstructCall()) {
            info.GetReturnValue().Set(info.This());
        } else {
            info.GetReturnValue().SetUndefined();
        }
    }

    void FunctionTemplate::get_instance_template(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, info.Holder()));
        auto result = wrapper->this_instance_template_holder(isolate);
        return info.GetReturnValue().Set(result);
    }

    void FunctionTemplate::get_prototype_template(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        JS_EXECUTE_RETURN(NOTHING, FunctionTemplate *, wrapper, FunctionTemplate::unwrap(isolate, info.Holder()));
        auto result = wrapper->this_prototype_template_holder(isolate);
        return info.GetReturnValue().Set(result);
    }
}