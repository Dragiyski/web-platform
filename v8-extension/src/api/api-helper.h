#ifndef V8EXT_API_HELPER_H
#define V8EXT_API_HELPER_H

#include "../js-helper.h"

#define DECLARE_API_WRAPPER_ANONYMOUS_DATA\
    namespace {\
        std::map<v8::Isolate*, v8::Global<v8::FunctionTemplate>> per_isolate_template;\
        std::map<v8::Isolate*, v8::Global<v8::Private>> per_isolate_private;\
        std::map<v8::Isolate*, v8::Global<v8::String>> per_isolate_name;\
    }

#define DECLARE_API_WRAPPER_CLASS_INITIALIZE(class_name)\
    v8::Maybe<void> class_name::initialize(v8::Isolate* isolate) {\
        v8::HandleScope scope(isolate);\
        \
        auto name = v8::String::NewFromUtf8Literal(isolate, #class_name);\
        auto local_private = v8::Private::New(isolate, name);\
        auto local_template = v8::FunctionTemplate::NewWithCache(isolate, constructor, local_private);\
        local_template->SetClassName(name);\
        local_template->InstanceTemplate()->SetInternalFieldCount(1);\
        \
        per_isolate_name.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple(isolate, name));\
        per_isolate_private.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple(isolate, local_private));\
        per_isolate_template.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple(isolate, local_template));\
        \
        JS_EXECUTE_IGNORE(VOID_NOTHING, class_name::initialize_template(isolate, local_template));\
        \
        return v8::JustVoid();\
    }

#define DECLARE_API_WRAPPER_ANONYMOUS_DATA_ACCESSORS(class_name)\
    v8::Local<v8::FunctionTemplate> class_name::get_template(v8::Isolate* isolate) {\
        auto it = per_isolate_template.find(isolate);\
        assert(it != per_isolate_template.end() && "get_template(): not found for this isolate");\
        return it->second.Get(isolate);\
    }\
    \
    v8::Local<v8::Private> class_name::get_private(v8::Isolate* isolate) {\
        auto it = per_isolate_private.find(isolate);\
        assert(it != per_isolate_private.end() && "get_private(): not found for this isolate");\
        return it->second.Get(isolate);\
    }\
    \
    v8::Local<v8::String> class_name::get_name(v8::Isolate* isolate) {\
        auto it = per_isolate_name.find(isolate);\
        assert(it != per_isolate_name.end() && "get_name(): not found for this isolate");\
        return it->second.Get(isolate);\
    }

#define DECLARE_API_WRAPPER_BODY(class_name)\
    DECLARE_API_WRAPPER_ANONYMOUS_DATA\
    DECLARE_API_WRAPPER_CLASS_INITIALIZE(class_name)\
    DECLARE_API_WRAPPER_ANONYMOUS_DATA_ACCESSORS(class_name)\

#define DECLARE_API_WRAPPER_HEAD \
    public:\
        static v8::Maybe<void> initialize(v8::Isolate *isolate);\
        static v8::Maybe<void> initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template);\
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate *isolate);\
        static v8::Local<v8::Private> get_private(v8::Isolate *isolate); \
        static v8::Local<v8::String> get_name(v8::Isolate *isolate);

#endif /* V8EXT_API_HELPER_H */