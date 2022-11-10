#ifndef V8EXT_SCRIPT_H
#define V8EXT_SCRIPT_H

#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <map>

class Script : node::ObjectWrap {
private:
    static std::map<v8::Isolate *, v8::Global<v8::FunctionTemplate>> class_template;
public:
    static v8::MaybeLocal<v8::FunctionTemplate> Template(v8::Local<v8::Context> context);
protected:
    static void Constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
    static v8::MaybeLocal<v8::Script> CreateScript(const v8::FunctionCallbackInfo<v8::Value> &info);
protected:
    v8::Persistent<v8::Script> m_script;
protected:
    Script(v8::Local<v8::Context>, v8::Local<v8::Script>);
};

#endif /* V8EXT_SCRIPT_H */
