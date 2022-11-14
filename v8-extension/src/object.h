#ifndef V8EXT_OBJECT_H
#define V8EXT_OBJECT_H

#include <v8.h>
#include <node_object_wrap.h>

namespace dragiyski::node_ext {

    /**
     * @brief Extended object wrapper
     * 
     * Extend the object wrapper functionality by storing the isolate wrapping this object.
     * When a NodeJS environment (main or worker) is freed, it clears all the C++ objects.
     * While we don't have to do that for every API-created V8 object, since isolate
     * clear its heap, if wrapping V8 related objects that might affect the isolate state
     * like the MicrotaskQueue (which is referenced by a linked-list in the isolate),
     * might lead to Debug Check (DCHECK) failure and a fatar error/crash.
     * 
     * ObjectWrap ensure this does not happen, by clearing out all its instances from the isolate.
     */
    class ObjectWrap : public node::ObjectWrap {
    public:
        static v8::Maybe<void> initialize(v8::Isolate *isolate);
        static void uninitialize(v8::Isolate *isolate);
    private:
        v8::Persistent<v8::Object> _container;
    public:
        void Wrap(v8::Local<v8::Object> holder, v8::Local<v8::Object> container);
        v8::Local<v8::Object> container(v8::Isolate *isolate) const;
    public:
        explicit ObjectWrap(v8::Isolate* isolate);
        virtual ~ObjectWrap();
    };

}

#endif /* V8EXT_OBJECT_H */
