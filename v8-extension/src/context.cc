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
    namespace {

        class Context : public node::ObjectWrap {

        };

        class PerIsolate {
        public:
            v8::Global<v8::ObjectTemplate> _template;
            v8::Global<v8::Private> _symbol;
            std::set<Context *> _context_set;
            PerIsolate() = default;
            PerIsolate(const PerIsolate &) = delete;
            PerIsolate(PerIsolate &&) = delete;
            static inline std::map<v8::Isolate *, PerIsolate> _map;
        public:
            static inline const std::map<v8::Isolate *, PerIsolate> &map() {
                return _map;
            }

            static v8::Maybe<void> init(v8::Isolate *isolate) {
                assert(_map.find(isolate) == _map.end());
                _map.emplace(std::piecewise_construct, std::make_tuple(isolate), std::make_tuple());
                return v8::JustVoid();
            }

            static inline void set_template(v8::Isolate *isolate, v8::Local<v8::ObjectTemplate> the_template) {
                auto it = _map.find(isolate);
                assert(it != _map.end());
                it->second._template.Reset(isolate, the_template);
            }

            static inline void set_symbol(v8::Isolate *isolate, v8::Local<v8::Private> the_symbol) {
                auto it = _map.find(isolate);
                assert(it != _map.end());
                it->second._symbol.Reset(isolate, the_symbol);
            }

            static inline v8::Local<v8::ObjectTemplate> get_template(v8::Isolate *isolate) {
                auto it = _map.find(isolate);
                assert(it != _map.end());
                return it->second._template.Get(isolate);
            }

            static inline v8::Local<v8::Private> get_symbol(v8::Isolate *isolate) {
                auto it = _map.find(isolate);
                assert(it != _map.end());
                return it->second._symbol.Get(isolate);
            }

            static inline bool is_valid_context(v8::Isolate *isolate, Context *object) {
                auto it = _map.find(isolate);
                assert(it != _map.end());
                return it->second._context_set.contains(object);
            }
        };
    }

    v8::Maybe<void> context_init(v8::Local<v8::Context> context) {
        auto isolate = context->GetIsolate();
        JS_EXECUTE_IGNORE(VOID_NOTHING, PerIsolate::init(isolate));

        v8::HandleScope scope(isolate);
        auto wrapper_template = v8::ObjectTemplate::New(isolate);
        wrapper_template->SetInternalFieldCount(1);
        PerIsolate::set_template(isolate, wrapper_template);

        auto wrapper_symbol = v8::Private::New(isolate);
        PerIsolate::set_symbol(isolate, wrapper_symbol);

        return v8::JustVoid();
    }

    void js_global_of(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        info.GetReturnValue().SetNull();
        if (info.Length() >= 1 && info[0]->IsObject()) {
            JS_EXECUTE_RETURN_HANDLE(NOTHING, v8::Context, creation_context, info[0].As<v8::Object>()->GetCreationContext());
            info.GetReturnValue().Set(creation_context->Global());
        }
    }

    void js_create_context(const v8::FunctionCallbackInfo<v8::Value> &info) {
        auto isolate = info.GetIsolate();
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();

        auto global_template = v8::FunctionTemplate::New(isolate, throw_illegal_constructor);
        global_template->InstanceTemplate()->SetInternalFieldCount(1);

        auto microtask_queue = v8::MicrotaskQueue::New(isolate, v8::MicrotasksPolicy::kAuto);

        auto context = v8::Context::New(
            isolate,
            nullptr,
            global_template,
            v8::MaybeLocal<v8::Value>(),
            v8::DeserializeEmbedderFieldsCallback(),
            microtask_queue;
        );
    }
}
