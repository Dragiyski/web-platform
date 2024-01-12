#ifndef JS_WRAPPER_HXX
#define JS_WRAPPER_HXX

#include <cassert>
#include <memory>
#include <node_object_wrap.h>
#include <typeinfo>
#include <v8.h>

#include "js-helper.hxx"
#include "js-string-table.hxx"

namespace js {

    template<class Class>
    class NativeObject {
    private:
        static
    };
    class Wrapper {
    public:
        static void initialize(v8::Isolate *isolate);
        static void uninitialize(v8::Isolate *isolate);

    public:
        static void dispose(v8::Isolate *isolate, Wrapper *wrapper);

    private:
        Shared<v8::Object> _holder;

    public:
        static v8::Local<v8::Private> get_this_symbol(v8::Isolate *isolate);
        static bool has_holder(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template);
        v8::Local<v8::Object> get_holder(v8::Isolate *isolate) const;
        static v8::MaybeLocal<v8::Object> get_holder(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template, const char *type_name);
        template<typename T>
        static T *Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> holder);
        template<typename T>
        static v8::Maybe<T *> Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template, const char *type_name = typeid(T).name());
        template<typename T>
        static v8::Maybe<T *> TryUnwrap(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template);
        template<typename T>
        static v8::Maybe<bool> IsWrapped(v8::Isolate *isolate, v8::Local<v8::Value> value);

    protected:
        void Wrap(v8::Isolate *isolate, v8::Local<v8::Object> holder);

    private:
        void MakeWeak();
        static void WeakCallback(const v8::WeakCallbackInfo<Wrapper> &info);
        template<typename T>
        static v8::MaybeLocal<v8::Object> resolve_self(v8::Isolate *isolate, v8::Local<v8::Object> self);

    protected:
        Wrapper() = default;
        Wrapper(const Wrapper &) = delete;
        Wrapper(Wrapper &&) = delete;

    public:
        virtual ~Wrapper();
    };

    template<typename T>
    v8::Maybe<bool> Wrapper::IsWrapped(v8::Isolate *isolate, v8::Local<v8::Value> value) {
        static constexpr const auto __function_return_type__ = v8::Nothing<bool>;
        static constexpr bool has_template_getter = requires() {
            { T::get_class_template(isolate) } -> std::convertible_to<v8::Local<v8::FunctionTemplate>>;
        };
        if (!value->IsObject()) {
            return v8::Just(false);
        }
        JS_EXPRESSION_RETURN(resolved, Wrapper::resolve_self<T>(isolate, value.As<v8::Object>()));
        if (!resolved->IsObject()) {
            return v8::Just(false);
        }
        auto holder = resolved;
        if constexpr (has_template_getter) {
            auto template_value = T::get_class_template(isolate);
            holder = resolved->FindInstanceInPrototypeChain(template_value);
        }
        return v8::Just(!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1);
    }

    template<typename T>
    T *Wrapper::Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> holder) {
        assert(!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1);
        auto wrapper_ptr = reinterpret_cast<Wrapper *>(holder->GetAlignedPointerFromInternalField(0));
        return dynamic_cast<T *>(wrapper_ptr);
    }

    template<typename T>
    v8::MaybeLocal<v8::Object> Wrapper::resolve_self(v8::Isolate *isolate, v8::Local<v8::Object> self) {
        using __function_return_type__ = v8::MaybeLocal<v8::Object>;
        static constexpr bool has_symbol = requires() {
            { T::get_class_symbol() } -> std::convertible_to<v8::Local<v8::Private>>;
        };
        v8::HandleScope scope(isolate);
        auto context = isolate->GetCurrentContext();
        v8::Local<v8::Private> symbol;
        if constexpr (has_symbol) {
            symbol = T::get_class_symbol();
        }
    do_until_resolved:
        v8::Local<v8::Value> value = self;
        if V8_UNLIKELY (value->IsProxy()) {
            value = value.As<v8::Proxy>()->GetTarget();
            if V8_LIKELY (value->IsObject()) {
                self = value.As<v8::Object>();
                goto do_until_resolved;
            }
        }
        if constexpr (has_symbol) {
            while (value->IsObject()) {
                auto object = value.As<v8::Object>();
                JS_EXPRESSION_RETURN(has_extension, object->HasPrivate(context, symbol));
                if (has_extension) {
                    JS_EXPRESSION_RETURN(extension, object->GetPrivate(context, symbol));
                    if V8_LIKELY (extension->IsObject() && !extension->SameValue(value)) {
                        self = extension.As<v8::Object>();
                        goto do_until_resolved;
                    }
                }
                value = object->GetPrototype();
            }
        }
        return self;
    }

    template<typename T>
    v8::Maybe<T *> Wrapper::Unwrap(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template, const char *type_name) {
        static constexpr const auto __function_return_type__ = v8::Nothing<T *>;
        JS_EXPRESSION_RETURN(resolved, Wrapper::resolve_self<T>(isolate, self));
        JS_EXPRESSION_RETURN(holder, Wrapper::get_holder(isolate, resolved, class_template, type_name));
        auto ptr = Wrapper::Unwrap<T>(isolate, holder);
        if V8_UNLIKELY(ptr == nullptr) {
            JS_THROW_ERROR(ReferenceError, isolate, "[", "object", " ", type_name, "]", " ", "no longer wraps a native object");
        }
        return v8::Just(ptr);
    };

    template<typename T>
    v8::Maybe<T *> Wrapper::TryUnwrap(v8::Isolate *isolate, v8::Local<v8::Object> self, v8::Local<v8::FunctionTemplate> class_template) {
        static constexpr const auto __function_return_type__ = v8::Nothing<T *>;
        JS_EXPRESSION_RETURN(resolved, Wrapper::resolve_self<T>(isolate, self));
        auto holder = self->FindInstanceInPrototypeChain(class_template);
        if V8_LIKELY(!holder.IsEmpty() && holder->IsObject() && holder->InternalFieldCount() >= 1) {
            auto ptr = Wrapper::Unwrap<T>(isolate, holder);
            if V8_LIKELY(ptr != nullptr) {
                return v8::Just(ptr);
            }
        }
        return v8::Just<T *>(nullptr);
    }
}

#endif /* JS_WRAPPER_HXX */
