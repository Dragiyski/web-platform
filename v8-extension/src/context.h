#ifndef V8EXT_CONTEXT_H
#define V8EXT_CONTEXT_H

#include <v8.h>

namespace dragiyski::node_ext {
    void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info);
}

#endif /* V8EXT_CONTEXT_H */
