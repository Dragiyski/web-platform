#include "template.h"

#include <cassert>
#include <map>
#include "../js-helper.h"

namespace dragiyski::node_ext {
    DECLARE_API_WRAPPER_BODY(Template);

    v8::Maybe<void> Template::initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template) {
        return v8::JustVoid();
    }

    void Template::constructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);

        JS_THROW_ERROR(NOTHING, isolate, TypeError, "Illegal constructor");
    }

    Template::Template(v8::Isolate *isolate) : ObjectWrap(isolate) {}
}
