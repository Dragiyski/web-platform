#ifndef V8EXT_API_HELPER_H
#define V8EXT_API_HELPER_H

#include "../js-helper.h"
#include <map>

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

#define DECLARE_API_WRAPPER_CLASS_INITIALIZE_MORE(class_name, more)\
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
        JS_EXECUTE_IGNORE(VOID_NOTHING, more(isolate))\
        JS_EXECUTE_IGNORE(VOID_NOTHING, class_name::initialize_template(isolate, local_template));\
        \
        return v8::JustVoid();\
    }

#define DECLARE_API_WRAPPER_CLASS_UNINITIALIZE(class_name)\
    void class_name::uninitialize(v8::Isolate* isolate) {\
        per_isolate_template.erase(isolate);\
        per_isolate_private.erase(isolate);\
        per_isolate_name.erase(isolate);\
    }

#define DECLARE_API_WRAPPER_CLASS_UNINITIALIZE_MORE(class_name, more)\
    void class_name::uninitialize(v8::Isolate* isolate) {\
        per_isolate_template.erase(isolate);\
        per_isolate_private.erase(isolate);\
        per_isolate_name.erase(isolate);\
        more(isolate);\
    }

#define DECLARE_API_WRAPPER_UNWRAP(class_name)\
    v8::Maybe<class_name *> class_name::unwrap(v8::Isolate *isolate, v8::Local<v8::Object> object) {\
        auto holder = object->FindInstanceInPrototypeChain(get_template(isolate));\
        if (holder.IsEmpty() || !holder->IsObject() || holder->InternalFieldCount() < 1) {\
            JS_THROW_ERROR(CPP_NOTHING(class_name *), isolate, TypeError, "Cannot convert value to '" #class_name "'");\
        }\
        auto wrapper = node::ObjectWrap::Unwrap<class_name>(holder);\
        if (wrapper == nullptr) {\
            JS_THROW_ERROR(CPP_NOTHING(class_name *), isolate, TypeError, "Object of type '" #class_name "' is already disposed");\
        }\
        return v8::Just(wrapper);\
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
    DECLARE_API_WRAPPER_CLASS_UNINITIALIZE(class_name)\
    DECLARE_API_WRAPPER_ANONYMOUS_DATA_ACCESSORS(class_name)\
    DECLARE_API_WRAPPER_UNWRAP(class_name)

#define DECLARE_API_WRAPPER_BODY_MORE(class_name, init_method, uninit_method)\
    DECLARE_API_WRAPPER_ANONYMOUS_DATA\
    DECLARE_API_WRAPPER_CLASS_INITIALIZE_MORE(class_name, init_method)\
    DECLARE_API_WRAPPER_CLASS_UNINITIALIZE_MORE(class_name, uninit_method)\
    DECLARE_API_WRAPPER_ANONYMOUS_DATA_ACCESSORS(class_name)\
    DECLARE_API_WRAPPER_UNWRAP(class_name)

#define DECLARE_API_WRAPPER_HEAD(class_name) \
    public:\
        static v8::Maybe<void> initialize(v8::Isolate *isolate);\
        static void uninitialize(v8::Isolate *isolate);\
        static v8::Maybe<void> initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template);\
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate *isolate);\
        static v8::Local<v8::Private> get_private(v8::Isolate *isolate);\
        static v8::Local<v8::String> get_name(v8::Isolate *isolate);\
        static v8::Maybe<class_name *> unwrap(v8::Isolate *isolate, v8::Local<v8::Object> object);

#endif /* V8EXT_API_HELPER_H */