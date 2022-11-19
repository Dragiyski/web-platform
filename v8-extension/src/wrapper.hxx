#ifndef JS_WRAPPER_HXX
#define JS_WRAPPER_HXX

#include <v8.h>
#include <node_object_wrap.h>
#include <typeinfo>
#include <memory>

#include "js-helper.hxx"
#include "js-string-table.hxx"

namespace js {
    class Wrapper : public std::enable_shared_from_this<Wrapper>, protected node::ObjectWrap {
    public:
        static void initialize(v8::Isolate* isolate);
        void uninitialize(v8::Isolate* isolate);
    private:
        Shared<v8::Object> _self;
    public:
        v8::Local<v8::Object> self(v8::Isolate* isolate) const;
        v8::Local<v8::Object> holder(v8::Isolate* isolate);
        v8::MaybeLocal<v8::Object> Wrap(v8::Isolate* isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template);
        template<typename T> static inline std::shared_ptr<T> Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template);

    private:
        static std::shared_ptr<Wrapper> find_shared_for_this(v8::Isolate *isolate, Wrapper *wrapper);
    protected:
        Wrapper(v8::Isolate* isolate, v8::Local<v8::Object> self);
        Wrapper(const Wrapper&) = delete;
        Wrapper(Wrapper&&) = delete;
    public:
        virtual ~Wrapper() = default;
    };

    template<typename T>
    std::shared_ptr<T> Wrapper::Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template) {
        using __function_return_type__ = std::shared_ptr<T>;

        v8::Local<v8::Object> holder = self->FindInstanceInPrototypeChain(class_template);
        if (holder.IsEmpty() || !holder->IsObject() || holder->InternalFieldCount() < 1) {
            JS_THROW_ERROR(TypeError, "The value is not of type '", typeid(T).name(), "'.")
        }

        auto raw_ptr = node::ObjectWrap::Unwrap<T>(self);
        auto wrapper_ptr = find_shared_for_this(isolate, raw_ptr);
        auto ptr = std::dynamic_pointer_cast<T>(wrapper_ptr);
    }
}

#endif /* JS_WRAPPER_HXX */
