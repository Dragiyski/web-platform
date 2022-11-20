#ifndef JS_WRAPPER_HXX
#define JS_WRAPPER_HXX

#include <v8.h>
#include <node_object_wrap.h>
#include <memory>
#include <cassert>

#include "js-helper.hxx"
#include "js-string-table.hxx"

namespace js {
    class Wrapper : public std::enable_shared_from_this<Wrapper> {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    private:
        Shared<v8::Object> _holder;
    public:
        v8::Local<v8::Object> get_holder(v8::Isolate* isolate);
        static v8::MaybeLocal<v8::Object> get_holder(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template);
        template<typename T>
        static std::shared_ptr<T> Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> holder);
        template<typename T>
        static v8::Maybe<std::shared_ptr<T>> Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template);
    protected:
        void Wrap(v8::Isolate *isolate, v8::Local<v8::Object> holder);
    private:
        static std::shared_ptr<Wrapper> find_shared_for_this(v8::Isolate *isolate, Wrapper *wrapper);
        void MakeWeak();
        static void WeakCallback(const v8::WeakCallbackInfo<Wrapper> &info);
    protected:
        Wrapper() = default;
        Wrapper(const Wrapper&) = delete;
        Wrapper(Wrapper&&) = delete;
    public:
        virtual ~Wrapper();
    };

    template<typename T>
    std::shared_ptr<T> Wrapper::Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> holder) {
        assert(!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1);
        auto wrapper_ptr = reinterpret_cast<Wrapper *>(holder->GetAlignedPointerFromInternalField(0));
        assert(wrapper_ptr != nullptr);
        auto wrapper = find_shared_for_this(isolate, wrapper_ptr);
        assert(wrapper.get() != nullptr);
        return std::dynamic_pointer_cast<T>(wrapper);
    }

    template<typename T>
    v8::Maybe<std::shared_ptr<T>> Wrapper::Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template) {
        using __function_return_type__ = v8::Maybe<std::shared_ptr<T>>;
        JS_EXPRESSION_RETURN(holder, Wrapper::get_holder(isolate, self, class_template));
        return v8::Just(Wrapper::Unwrap<T>(isolate, holder));
    };
}

#endif /* JS_WRAPPER_HXX */
