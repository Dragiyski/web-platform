#include "function.h"
#include "js-helper.h"

#include "string-table.h"

namespace dragiyski::node_ext {

    void js_function_set_name(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();
        if (info.Length() < 2) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 2, " arguments, got ", info.Length());
        }
        if (!info[0]->IsFunction()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
        }
        if (!info[1]->IsString()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[1] to be a string, got ", info[1]);
        }
        info[0].As<v8::Function>()->SetName(info[1].As<v8::String>());
    }

    void js_function_get_name(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();
        if (info.Length() < 1) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected ", 1, " arguments, got ", info.Length());
        }
        if (!info[0]->IsFunction()) {
            JS_THROW_ERROR(NOTHING, context, TypeError, "Expected arguments[0] to be a function, got ", info[0]);
        }
        info.GetReturnValue().Set(info[0].As<v8::Function>()->GetName());
    }

    void throw_illegal_constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        v8::HandleScope scope(info.GetIsolate());
        auto context = info.GetIsolate()->GetCurrentContext();
        JS_THROW_ERROR(NOTHING, context, TypeError, "Illegal constructor");
    }

    std::unique_ptr<v8::ScriptCompiler::Source> source_from_object(v8::Local<v8::Context> context, v8::Local<v8::Object> options) {
        auto isolate = context->GetIsolate();

        v8::Local<v8::String> source;
        {
            JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "source");
            if (!js_value->IsString()) {
                JS_THROW_ERROR(nullptr, context, TypeError, "Expected option 'source' to be a string.");
            }
            source = js_value.As<v8::String>();
        }
        v8::Local<v8::Value> location;
        {
            JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "location");
            if (!js_value->IsNullOrUndefined()) {
                JS_EXECUTE_RETURN_HANDLE(nullptr, v8::String, value, js_value->ToString(context));
                location = value;
            }
        }

        if (!location.IsEmpty()) {
            int line_offset = 0, column_offset = 0;
            v8::Local<v8::Value> source_map_url;
            int script_id = -1;
            bool is_shared_cross_origin = false, is_opaque = false, is_wasm = false, is_module = false;
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "lineOffset");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(nullptr, int, value, js_value->Int32Value(context));
                    line_offset = value;
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "columnOffset");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(nullptr, int, value, js_value->Int32Value(context));
                    column_offset = value;
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "scriptId");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN(nullptr, int, value, js_value->Int32Value(context));
                    script_id = value;
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "sourceMapUrl");
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXECUTE_RETURN_HANDLE(nullptr, v8::String, value, js_value->ToString(context));
                    source_map_url = value;
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "isSharedCrossOrigin");
                if (!js_value->IsNullOrUndefined()) {
                    is_shared_cross_origin = js_value->BooleanValue(isolate);
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "isOpaque");
                if (!js_value->IsNullOrUndefined()) {
                    is_opaque = js_value->BooleanValue(isolate);
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "isWASM");
                if (!js_value->IsNullOrUndefined()) {
                    is_wasm = js_value->BooleanValue(isolate);
                }
            }
            {
                JS_OBJECT_GET_LITERAL_KEY(nullptr, js_value, context, options, "isModule");
                if (!js_value->IsNullOrUndefined()) {
                    is_module = js_value->BooleanValue(isolate);
                }
            }

            v8::ScriptOrigin origin(isolate, location, line_offset, column_offset, is_shared_cross_origin, script_id, source_map_url, is_opaque, is_wasm, is_module);
            return std::make_unique<v8::ScriptCompiler::Source>(source, origin);
        } else {
            return std::make_unique<v8::ScriptCompiler::Source>(source);
        }
    }

}