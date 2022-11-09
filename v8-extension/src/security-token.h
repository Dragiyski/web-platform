#ifndef V8EXT_SECURITY_TOKEN_H
#define V8EXT_SECURITY_TOKEN_H

#include <v8.h>

void js_get_security_token(const v8::FunctionCallbackInfo<v8::Value> &info);
void js_set_security_token(const v8::FunctionCallbackInfo<v8::Value> &info);
void js_use_default_security_token(const v8::FunctionCallbackInfo<v8::Value> &info);

#endif /* V8EXT_SECURITY_TOKEN_H */
