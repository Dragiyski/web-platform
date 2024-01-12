#include "user-context.hxx"

#include "../js-string-table.hxx"
#include <map>

namespace dragiyski::node_ext {
    using namespace js;

    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_template;
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void UserContext::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = ::js::StringTable::Get(isolate, "UserContext");
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
        class_template->SetClassName(class_name);
        class_template->Inherit(Context::get_class_template(isolate));
        auto prototype_template = class_template->PrototypeTemplate();
        auto signature = v8::Signature::New(isolate, class_template);
        {
            auto name = StringTable::Get(isolate, "maxEntryTime");
            auto getter = v8::FunctionTemplate::New(
                isolate,
                prototype_get_max_entry_time,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasNoSideEffect
            );
            getter->SetClassName(name);
            auto setter = v8::FunctionTemplate::New(
                isolate,
                prototype_set_max_entry_time,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasSideEffectToReceiver
            );
            setter->SetClassName(name);
            prototype_template->SetAccessorProperty(name, getter, setter, JS_PROPERTY_ATTRIBUTE_SEAL);
        }
        {
            auto name = StringTable::Get(isolate, "maxUserTime");
            auto getter = v8::FunctionTemplate::New(
                isolate,
                prototype_get_max_user_time,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasNoSideEffect
            );
            getter->SetClassName(name);
            auto setter = v8::FunctionTemplate::New(
                isolate,
                prototype_set_max_user_time,
                {},
                signature,
                0,
                v8::ConstructorBehavior::kThrow,
                v8::SideEffectType::kHasSideEffectToReceiver
            );
            setter->SetClassName(name);
            prototype_template->SetAccessorProperty(name, getter, setter, JS_PROPERTY_ATTRIBUTE_SEAL);
        }
    }
}