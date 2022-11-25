#ifndef JS_WRAPPER_HXX
#define JS_WRAPPER_HXX

#include <v8.h>
#include <node_object_wrap.h>
#include <memory>
#include <cassert>
#include <typeinfo>

#include "js-helper.hxx"
#include "js-string-table.hxx"

namespace js {
    class Wrapper {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static void dispose(v8::Isolate *isolate, Wrapper *wrapper);
    private:
        Shared<v8::Object> _holder;
    public:
        v8::Local<v8::Object> get_holder(v8::Isolate* isolate);
        static v8::MaybeLocal<v8::Object> get_holder(v8::Isolate* isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template, const char *type_name);
        template<typename T>
        static T* Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> holder);
        template<typename T>
        static v8::Maybe<T*> Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template, const char *type_name = typeid(T).name());
    protected:
        void Wrap(v8::Isolate* isolate, v8::Local<v8::Object> holder);
    private:
        void MakeWeak();
        static void WeakCallback(const v8::WeakCallbackInfo<Wrapper>& info);
    protected:
        Wrapper() = default;
        Wrapper(const Wrapper&) = delete;
        Wrapper(Wrapper&&) = delete;
    public:
        virtual ~Wrapper();
    };

    template<typename T>
    T* Wrapper::Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> holder) {
        assert(!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1);
        auto wrapper_ptr = reinterpret_cast<Wrapper*>(holder->GetAlignedPointerFromInternalField(0));
        return dynamic_cast<T*>(wrapper_ptr);
    }

    template<typename T>
    v8::Maybe<T*> Wrapper::Unwrap(v8::Isolate* isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template, const char *type_name) {
        static constexpr auto __function_return_type__ = v8::Nothing<T*>;
        JS_EXPRESSION_RETURN(holder, Wrapper::get_holder(isolate, self, class_template, type_name));
        auto ptr = Wrapper::Unwrap<T>(isolate, holder);
        if (ptr == nullptr) {
            JS_THROW_ERROR(ReferenceError, isolate, "[object ", type_name, "] no longer wraps a native object");
        }
        return v8::Just(ptr);
    };
}

#endif /* JS_WRAPPER_HXX */
