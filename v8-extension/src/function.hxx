#ifndef NODE_EXT_FUNCTION_HXX
#define NODE_EXT_FUNCTION_HXX

#include <memory>
#include "js-helper.hxx"

namespace dragiyski::node_ext {
    std::unique_ptr<v8::ScriptCompiler::Source> source_from_object(v8::Local<v8::Context>, v8::Local<v8::Object>);
}

#endif /* NODE_EXT_FUNCTION_HXX */