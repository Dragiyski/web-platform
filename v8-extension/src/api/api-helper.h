#ifndef V8EXT_API_HELPER_H
#define V8EXT_API_HELPER_H

#include "../js-helper.h"
#include <map>

#define DECLARE_API_WRAPPER_ANONYMOUS_DATA\
    namespace {\
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_template;\
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_symbol_constructor;\
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_symbol_this;\
        std::map<v8::Isolate*, Shared<v8::String>> per_isolate_name;\
    }

#define DECLARE_API_WRAPPER_CLASS_INITIALIZE_BASE(class_name)\
        v8::HandleScope scope(isolate);\
        \
        auto name = v8::String::NewFromUtf8Literal(isolate, #class_name);\
        auto local_symbol_constructor = v8::Private::New(isolate, name);\
        auto local_symbol_this = v8::Private::New(isolate, name);\
        auto local_template = v8::FunctionTemplate::NewWithCache(isolate, constructor, local_symbol_constructor);\
        local_template->SetClassName(name);\
        local_template->InstanceTemplate()->SetInternalFieldCount(1);\
        \
        per_isolate_name.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple(isolate, name));\
        per_isolate_symbol_constructor.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple(isolate, local_symbol_constructor));\
        per_isolate_symbol_this.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple(isolate, local_symbol_this));\
        per_isolate_template.emplace(std::piecewise_construct, std::forward_as_tuple(isolate), std::forward_as_tuple(isolate, local_template));\

#define DECLARE_API_WRAPPER_CLASS_INITIALIZE(class_name)\
    Maybe<void> class_name::initialize(v8::Isolate* isolate) {\
        DECLARE_API_WRAPPER_CLASS_INITIALIZE_BASE(class_name)\
        \
        JS_EXECUTE_IGNORE(VOID_NOTHING, class_name::initialize_template(isolate, local_template));\
        \
        return v8::JustVoid();\
    }

#define DECLARE_API_WRAPPER_CLASS_INITIALIZE_MORE(class_name, more)\
    Maybe<void> class_name::initialize(v8::Isolate* isolate) {\
        DECLARE_API_WRAPPER_CLASS_INITIALIZE_BASE(class_name)\
        \
        JS_EXECUTE_IGNORE(VOID_NOTHING, more(isolate))\
        JS_EXECUTE_IGNORE(VOID_NOTHING, class_name::initialize_template(isolate, local_template));\
        \
        return v8::JustVoid();\
    }

#define DECLARE_API_WRAPPER_CLASS_UNINITIALIZE_BASE(class_name)\
        per_isolate_template.erase(isolate);\
        per_isolate_symbol_constructor.erase(isolate);\
        per_isolate_symbol_this.erase(isolate);\
        per_isolate_name.erase(isolate);\

#define DECLARE_API_WRAPPER_CLASS_UNINITIALIZE(class_name)\
    void class_name::uninitialize(v8::Isolate* isolate) {\
        DECLARE_API_WRAPPER_CLASS_UNINITIALIZE_BASE(class_name)\
    }

#define DECLARE_API_WRAPPER_CLASS_UNINITIALIZE_MORE(class_name, more)\
    void class_name::uninitialize(v8::Isolate* isolate) {\
        DECLARE_API_WRAPPER_CLASS_UNINITIALIZE_BASE(class_name)\
        more(isolate);\
    }

#define DECLARE_API_WRAPPER_UNWRAP(class_name)\
    Maybe<class_name *> class_name::unwrap(v8::Isolate *isolate, v8::Local<v8::Object> object) {\
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

#define DECLARE_API_WRAPPER_PRIVATE_SYMBOL(class_name, symbol_name)\
    Local<v8::Private> class_name::symbol_name(v8::Isolate* isolate) {\
        auto it = per_isolate_ ## symbol_name.find(isolate);\
        assert(it != per_isolate_ ## symbol_name.end() && (#symbol_name "(): not found for this isolate"));\
        return it->second.Get(isolate);\
    }\

#define DECLARE_API_WRAPPER_ANONYMOUS_DATA_ACCESSORS(class_name)\
    Local<v8::FunctionTemplate> class_name::get_template(v8::Isolate* isolate) {\
        auto it = per_isolate_template.find(isolate);\
        assert(it != per_isolate_template.end() && "get_template(): not found for this isolate");\
        return it->second.Get(isolate);\
    }\
    DECLARE_API_WRAPPER_PRIVATE_SYMBOL(class_name, symbol_constructor)\
    DECLARE_API_WRAPPER_PRIVATE_SYMBOL(class_name, symbol_this)\
    \
    Local<v8::String> class_name::get_name(v8::Isolate* isolate) {\
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
        static Maybe<void> initialize(v8::Isolate *isolate);\
        static void uninitialize(v8::Isolate *isolate);\
        static Maybe<class_name *> unwrap(v8::Isolate *isolate, v8::Local<v8::Object> object);\
        static Local<v8::FunctionTemplate> get_template(v8::Isolate *isolate);\
        static Local<v8::Private> symbol_constructor(v8::Isolate *isolate);\
        static Local<v8::Private> symbol_this(v8::Isolate *isolate);\
        static Local<v8::String> get_name(v8::Isolate *isolate);\
    protected:\
        static Maybe<void> initialize_template(v8::Isolate *isolate, v8::Local<v8::FunctionTemplate> class_template);

#endif /* V8EXT_API_HELPER_H */