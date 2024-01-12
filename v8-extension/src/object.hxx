#ifndef JS_OBJECT_HXX
#define JS_OBJECT_HXX

#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <typeinfo>
#include <v8.h>

#include "js-helper.hxx"
#include "js-string-table.hxx"

namespace js {
    class ObjectBase {
    private:
        Shared<v8::Object> _interface;

    protected:
        virtual void set_interface(v8::Isolate *isolate, v8::Local<v8::Object> target);
        virtual void clear_interface(v8::Isolate *isolate);
        virtual void on_interface_gc(v8::Isolate *isolate);

    public:
        virtual v8::Local<v8::Object> get_interface(v8::Isolate *isolate) const;

    private:
        static void weak_callback(const v8::WeakCallbackInfo<ObjectBase> &info);

    protected:
        ObjectBase() = default;
        ObjectBase(const ObjectBase &) = default;
        ObjectBase(ObjectBase &&) = default;

    public:
        virtual ~ObjectBase() = default;
    };

    template<class Class>
    class Object : public virtual ObjectBase {
    private:
        static std::map<v8::Isolate *, std::set<Object<Class> *>> per_isolate_object_set;

    public:
        static void initialize(v8::Isolate *isolate);
        static void uninitialize(v8::Isolate *isolate);

    protected:
        virtual void on_interface_gc(v8::Isolate *isolate);
        virtual void set_interface(v8::Isolate *isolate, v8::Local<v8::Object> target);
        virtual void clear_interface(v8::Isolate *isolate);
        // This might need to be Maybe<Class *> if the context is involved. It appears we can both move over prototype and proxy target without context though.
    public:
        static Class *get_implementation(v8::Isolate *isolate, v8::Local<v8::Object> target);
        static Class *get_own_implementation(v8::Isolate *isolate, v8::Local<v8::Object> target);
        static bool is_implementation(v8::Isolate *isolate, const Class *);

    protected:
        Object() = default;
        Object(const Object<Class> &) = default;
        Object(Object<Class> &&) = default;

    public:
        virtual ~Object() = default;
    };

    template<class Class>
    std::map<v8::Isolate *, std::set<Object<Class> *>> Object<Class>::per_isolate_object_set;

    template<class Class>
    inline void Object<Class>::initialize(v8::Isolate *isolate) {
        assert(!per_isolate_object_set.contains(isolate));
        per_isolate_object_set.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple()
        );
    }

    template<class Class>
    inline void Object<Class>::uninitialize(v8::Isolate *isolate) {
        assert(per_isolate_object_set.contains(isolate));
        for (auto *object : per_isolate_object_set[isolate]) {
            delete object;
        }
        per_isolate_object_set.erase(isolate);
    }

    template<class Class>
    inline void Object<Class>::set_interface(v8::Isolate *isolate, v8::Local<v8::Object> target) {
        assert(!target.IsEmpty() && target->IsObject() && target->InternalFieldCount() >= 1);
        assert(per_isolate_object_set.contains(isolate));
        assert(!per_isolate_object_set[isolate].contains(this));
        target->SetAlignedPointerInInternalField(0, this);
        per_isolate_object_set[isolate].insert(this);
        ObjectBase::set_interface(isolate, target);
    }

    template<class Class>
    inline void Object<Class>::clear_interface(v8::Isolate *isolate) {
        assert(per_isolate_object_set.contains(isolate));
        per_isolate_object_set[isolate].erase(this);
        ObjectBase::clear_interface(isolate);
    }

    template<class Class>
    inline void Object<Class>::on_interface_gc(v8::Isolate *isolate) {
        assert(per_isolate_object_set.contains(isolate));
        per_isolate_object_set[isolate].erase(this);
        ObjectBase::on_interface_gc(isolate);
    }

    template<class Class>
    inline Class *Object<Class>::get_implementation(v8::Isolate *isolate, v8::Local<v8::Object> target) {
        v8::Local<v8::Value> value = target;
        while (!value.IsEmpty() && value->IsObject()) {
            auto object = value.As<v8::Object>();
            if V8_UNLIKELY (object->IsProxy()) {
                value = object.As<v8::Proxy>()->GetTarget();
            } else if V8_LIKELY (object->InternalFieldCount() >= 1) {
                auto interface = reinterpret_cast<Class *>(object->GetAlignedPointerFromInternalField(0));
                if V8_LIKELY (per_isolate_object_set[isolate].contains(interface)) {
                    return interface;
                }
                // In case this is a wrapper of another type or from another library, we do not need to search the prototype anymore.
                // Extending our wrapper will be only pure JavaScript objects (internal field count = 0)
                return nullptr;
            } else {
                value = object->GetPrototype();
            }
        }
        return nullptr;
    }

    template<class Class>
    inline Class *Object<Class>::get_own_implementation(v8::Isolate *isolate, v8::Local<v8::Object> target) {
        if V8_LIKELY (!target.IsEmpty() && target->IsObject() && target->InternalFieldCount() >= 1) {
            auto interface = reinterpret_cast<Class *>(target->GetAlignedPointerFromInternalField(0));
            if V8_LIKELY (per_isolate_object_set[isolate].contains(interface)) {
                return interface;
            }
        }
        return nullptr;
    }

    template<class Class>
    inline bool Object<Class>::is_implementation(v8::Isolate *isolate, const Class *interface) {
        return per_isolate_object_set[isolate].contains(interface);
    }

    v8::MaybeLocal<v8::String> type_of(v8::Local<v8::Context> context, v8::Local<v8::Value> value);
}

#endif /* JS_OBJECT_HXX */