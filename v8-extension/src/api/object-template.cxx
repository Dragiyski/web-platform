#include "object-template.hxx"

#include "context.hxx"

#include "../js-string-table.hxx"
#include "../error-message.hxx"
#include <map>
#include <vector>

namespace dragiyski::node_ext {
    using namespace js;
    namespace {
        std::map<v8::Isolate*, Shared<v8::FunctionTemplate>> per_isolate_class_template;
        std::map<v8::Isolate*, Shared<v8::Private>> per_isolate_class_symbol;
    }

    void ObjectTemplate::initialize(v8::Isolate* isolate) {
        assert(!per_isolate_class_template.contains(isolate));
        assert(!per_isolate_class_symbol.contains(isolate));

        auto class_name = ::js::StringTable::Get<"FunctionTemplate">(isolate);
        auto class_cache = v8::Private::New(isolate, class_name);
        auto class_template = v8::FunctionTemplate::NewWithCache(
            isolate,
            constructor,
            class_cache
        );
        class_template->SetClassName(class_name);

        auto signature = v8::Signature::New(isolate, class_template);
        auto prototype_template = class_template->PrototypeTemplate();
        
        class_template->ReadOnlyPrototype();
        class_template->InstanceTemplate()->SetInternalFieldCount(1);

        per_isolate_class_symbol.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_cache)
        );

        per_isolate_class_template.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(isolate),
            std::forward_as_tuple(isolate, class_template)
        );
    }

    void ObjectTemplate::uninitialize(v8::Isolate* isolate) {
        per_isolate_class_template.erase(isolate);
        per_isolate_class_symbol.erase(isolate);
    }

    v8::Local<v8::FunctionTemplate> ObjectTemplate::get_class_template(v8::Isolate* isolate) {
        assert(per_isolate_class_template.contains(isolate));
        return per_isolate_class_template[isolate].Get(isolate);
    }

    v8::Local<v8::Private> ObjectTemplate::get_class_symbol(v8::Isolate* isolate) {
        assert(per_isolate_class_symbol.contains(isolate));
        return per_isolate_class_symbol[isolate].Get(isolate);
    }
}
