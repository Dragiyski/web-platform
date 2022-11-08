#include "native-function.h"
#include "js-helper.h"

void v8_function_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    v8::Local<v8::Array> call_arguments;
    {
        v8::Local<v8::Value> array_arguments[info.Length()];
        for (decltype(info.Length()) i = 0; i < info.Length(); ++i) {
            array_arguments[i] = info[i];
        }
        call_arguments = v8::Array::New(isolate, array_arguments, info.Length());
    }
    auto call_this = info.This();
    auto call_target = info.NewTarget();
    v8::Local<v8::Value> args[] = { call_this, call_arguments, call_target };
    v8::MaybeLocal<v8::Value> maybe_result;
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, result, info.Data().As<v8::Function>()->Call(context, v8::Undefined(isolate), 3, args));
    info.GetReturnValue().Set(result);
}

void js_create_native_function(const v8::FunctionCallbackInfo<v8::Value>& info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be an object, got ", info[0]);
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_context, ToString(context, "context"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_name, ToString(context, "name"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_code, ToString(context, "code"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_location, ToString(context, "location"));
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::String, string_length, ToString(context, "length"));
    auto options = info[0].As<v8::Object>();

    v8::Local<v8::String> location;
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, value, options->Get(context, string_location));
        if (!value->String()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option '", string_location, "' to be a string, got ", value);
        }
        callee = value.As<v8::Function>();
    }

    v8::ConstructorBehavior constructor_behavior = v8::ConstructorBehavior::kAllow;
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, value, options->Get(context, string_pure));
        if (value->BooleanValue(isolate)) {
            constructor_behavior = v8::ConstructorBehavior::kThrow;
        }
    }

    v8::Local<v8::Context> callee_context = context;
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, value, options->Get(context, string_context));
        if (!value->IsNullOrUndefined()) {
            if (!value->IsObject()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option '", string_context, "' to be an object, got ", value);
            }
            v8::MaybeLocal<v8::Context> creation_context;
            {
                v8::TryCatch try_catch(isolate);
                creation_context = value.As<v8::Object>()->GetCreationContext();
                if (creation_context.IsEmpty()) {
                    if (try_catch.HasCaught() || !try_catch.CanContinue() || try_catch.HasTerminated()) {
                        try_catch.ReThrow();
                        return;
                    }
                    // In case object has no creation context and it is not an error...
                }
            }
            if (creation_context.IsEmpty()) {
                JS_THROW_ERROR(NOTHING, context, Error, "Unable to retrieve the creation context for object specified at option 'context'");
            }
            callee_context = creation_context.ToLocalChecked();
        }
    }

    int length = 0;
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_value, options->Get(context, string_pure));
        if (!js_value->IsNullOrUndefined()) {
            JS_EXECUTE_RETURN(NOTHING, int, value, js_value->Int32Value(context));
            length = value;
        }
    }

    v8::Local<v8::String> name;
    {
        JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Value, js_value, options->Get(context, string_pure));
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsString()) {
                JS_THROW_ERROR(NOTHING, context, TypeError, "Expected option '", string_name, "' to be a string, got ", js_value);
            }
            name = js_value.As<v8::String>();
        }
    }

    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Function, caller, v8::Function::New(callee_context, v8_function_callback, callee, length, constructor_behavior));
    if (!name.IsEmpty()) {
        callee->SetName(name);
    }
    info.GetReturnValue().Set(caller);
}