#include "script.h"
#include "js-helper.h"
#include "function.h"

std::map<v8::Isolate *, v8::Global<v8::FunctionTemplate>> Script::class_template;

v8::MaybeLocal<v8::FunctionTemplate> Script::Template(v8::Local<v8::Context> context) {
    auto isolate = context->GetIsolate();
    auto location = class_template.find(isolate);
    if (location != class_template.end()) {
        auto result = location->second.Get(isolate);
        return result;
    }
    auto tpl = v8::FunctionTemplate::New(isolate, Constructor, v8::Local<v8::Value>(), v8::Local<v8::Signature>());
    JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::FunctionTemplate), v8::String, name, ToString(context, "Script"));
    tpl->SetClassName(name);
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    class_template.insert(std::make_pair(isolate, v8::Global<v8::FunctionTemplate>(isolate, tpl)));
    return tpl;
}

void Script::Constructor(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (!info.IsConstructCall()) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Class constructor Script cannot be invoked without 'new'");
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::FunctionTemplate, class_template, Template(context));
    auto holder = info.This()->FindInstanceInPrototypeChain(class_template);
    if (holder.IsEmpty() || holder->InternalFieldCount() < 1) {
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }
    JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Script, js_script, CreateScript(info));
    auto wrapper = new Script(context, js_script);
    wrapper->Wrap(holder);
    info.GetReturnValue().Set(info.This());
}

v8::MaybeLocal<v8::Script> CreateScript(const v8::FunctionCallbackInfo<v8::Value> &info) {
    auto isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    auto context = isolate->GetCurrentContext();
    if (info.Length() < 1) {
        JS_THROW_ERROR(JS_NOTHING(v8::Script), context, TypeError, "Expected ", 1, " argument, got ", info.Length());
    }
    if (!info[0]->IsObject()) {
        JS_THROW_ERROR(JS_NOTHING(v8::Script), context, TypeError, "Expected arguments[0] to be an object.");
    }
    auto options = info[0].As<v8::Object>();

    v8::Local<v8::Context> script_context = context;
    {
        JS_OBJECT_GET_KEY_HANDLE(JS_NOTHING(v8::Script), v8::Object, js_value, context, options, "context");
        if (!js_value->IsNullOrUndefined()) {
            if (!js_value->IsObject()) {
                JS_THROW_ERROR(JS_NOTHING(v8::Script), context, TypeError, "Expected option 'context' to be an object.");
            }
            JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Script), v8::Context, value, js_value->GetCreationContext());
            script_context = value;
        }
    }

    auto source = source_from_object(context, options);
    if (!source) {
        return JS_NOTHING(v8::Script);
    }
    JS_EXECUTE_RETURN_HANDLE(JS_NOTHING(v8::Script), v8::Script, result, v8::ScriptCompiler::Compile(script_context, source.get(), v8::ScriptCompiler::kEagerCompile));
    return result;
}
