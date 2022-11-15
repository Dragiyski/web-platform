#include "context.h"
#include "js-helper.h"
#include "function.h"

#include <mutex>
#include <map>
#include <set>
#include <optional>
#include <node.h>
#include <node_object_wrap.h>
#include <cassert>

namespace dragiyski::node_ext {

    void js_global_of(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        info.GetReturnValue().SetNull();
        if (info.Length() >= 1 && info[0]->IsObject()) {
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, creation_context, info[0].As<v8::Object>()->GetCreationContext());
            info.GetReturnValue().Set(creation_context->Global());
        }
    }

}
