#ifndef NODE_EXT_API_FROZEN_MAP_HXX
#define NODE_EXT_API_FROZEN_MAP_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../object.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    class FrozenMap : public Object<FrozenMap> {
    public:
        class Iterator : public Object<Iterator> {
        friend class FrozenMap;
        public:
            static void initialize(v8::Isolate* isolate);
            static void uninitialize(v8::Isolate* isolate);
        public:
            static v8::Local<v8::ObjectTemplate> get_template(v8::Isolate* isolate);
        public:
            static void prototype_next(const v8::FunctionCallbackInfo<v8::Value>& info);
        private:
            Shared<v8::Array> _key_value;
            decltype(std::declval<v8::Array>().Length()) _index;
            bool _with_key, _with_value;
        protected:
            Iterator() = default;
            Iterator(const Iterator&) = delete;
            Iterator(Iterator&&) = delete;
        public:
            virtual ~Iterator() override = default;
        };
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_template(v8::Isolate* isolate);
    protected:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_get(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_has(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_size(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_entries(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_keys(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void prototype_values(const v8::FunctionCallbackInfo<v8::Value>& info);
    public:
        static v8::MaybeLocal<v8::Object> Create(v8::Local<v8::Context> context, v8::Local<v8::Map> map);
    private:
        Shared<v8::Map> _map;
    public:
        v8::Local<v8::Map> get_map(v8::Isolate *isolate);
    protected:
        FrozenMap() = default;
        FrozenMap(const FrozenMap&) = delete;
        FrozenMap(FrozenMap&&) = delete;
    public:
        virtual ~FrozenMap() override = default;
    };
}

#endif /* NODE_EXT_API_FROZEN_MAP_HXX */