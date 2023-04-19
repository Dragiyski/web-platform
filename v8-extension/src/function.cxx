#include "function.hxx"
#include "js-string-table.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    std::unique_ptr<v8::ScriptCompiler::Source> source_from_object(v8::Local<v8::Context> context, v8::Local<v8::Object> options) {
        using __function_return_type__ = std::unique_ptr<v8::ScriptCompiler::Source>;
        auto isolate = context->GetIsolate();

        v8::Local<v8::String> source;
        {
            auto name = StringTable::Get(isolate, "source");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsString()) {
                JS_THROW_ERROR(TypeError, isolate, "Expected option 'source' to be a string.");
            }
            source = js_value.As<v8::String>();
        }
        v8::Local<v8::Value> location;
        {
            auto name = StringTable::Get(isolate, "location");
            JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
            if (!js_value->IsNullOrUndefined()) {
                JS_EXPRESSION_RETURN(value, js_value->ToString(context));
                location = value;
            }
        }

        if (!location.IsEmpty()) {
            int line_offset = 0, column_offset = 0;
            v8::Local<v8::Value> source_map_url;
            int script_id = -1;
            bool is_shared_cross_origin = false, is_opaque = false, is_wasm = false, is_module = false;
            {
                auto name = StringTable::Get(isolate, "lineOffset");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXPRESSION_RETURN(value, js_value->Int32Value(context));
                    line_offset = value;
                }
            }
            {
                auto name = StringTable::Get(isolate, "columnOffset");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXPRESSION_RETURN(value, js_value->Int32Value(context));
                    column_offset = value;
                }
            }
            {
                auto name = StringTable::Get(isolate, "scriptId");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXPRESSION_RETURN(value, js_value->Int32Value(context));
                    script_id = value;
                }
            }
            {
                auto name = StringTable::Get(isolate, "sourceMapUrl");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    JS_EXPRESSION_RETURN(value, js_value->ToString(context));
                    source_map_url = value;
                }
            }
            {
                auto name = StringTable::Get(isolate, "isSharedCrossOrigin");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    is_shared_cross_origin = js_value->BooleanValue(isolate);
                }
            }
            {
                auto name = StringTable::Get(isolate, "isOpaque");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    is_shared_cross_origin = js_value->BooleanValue(isolate);
                }
            }
            {
                auto name = StringTable::Get(isolate, "isWASM");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    is_shared_cross_origin = js_value->BooleanValue(isolate);
                }
            }
            {
                auto name = StringTable::Get(isolate, "isModule");
                JS_EXPRESSION_RETURN(js_value, options->Get(context, name));
                if (!js_value->IsNullOrUndefined()) {
                    is_shared_cross_origin = js_value->BooleanValue(isolate);
                }
            }

            v8::ScriptOrigin origin(isolate, location, line_offset, column_offset, is_shared_cross_origin, script_id, source_map_url, is_opaque, is_wasm, is_module);
            return std::make_unique<v8::ScriptCompiler::Source>(source, origin);
        } else {
            return std::make_unique<v8::ScriptCompiler::Source>(source);
        }
    }
}