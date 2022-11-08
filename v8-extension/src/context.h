#ifndef V8EXT_CONTEXT_H
#define V8EXT_CONTEXT_H

#include <v8.h>

void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info);

void js_create_context(const v8::FunctionCallbackInfo<v8::Value> &info);

#endif /* V8EXT_CONTEXT_H */
