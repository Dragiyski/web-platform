#ifndef V8EXT_FUNCTION_H
#define V8EXT_FUNCTION_H

#include <v8.h>
#include <optional>

void js_function_set_name(const v8::FunctionCallbackInfo<v8::Value> &info);

void js_function_get_name(const v8::FunctionCallbackInfo<v8::Value> &info);

std::unique_ptr<v8::ScriptCompiler::Source> source_from_object(v8::Local<v8::Context>, v8::Local<v8::Object>);

#endif /* V8EXT_FUNCTION_H */
