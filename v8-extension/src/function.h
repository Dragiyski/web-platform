#ifndef V8EXT_FUNCTION_H
#define V8EXT_FUNCTION_H

#include <v8.h>

void js_function_set_name(const v8::FunctionCallbackInfo<v8::Value> &info);

void js_function_get_name(const v8::FunctionCallbackInfo<v8::Value> &info);

#endif /* V8EXT_FUNCTION_H */
