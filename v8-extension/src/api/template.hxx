#ifndef NODE_EXT_API_TEMPLATE_HXX
#define NODE_EXT_API_TEMPLATE_HXX

#include <v8.h>
#include "../js-helper.hxx"
#include "../wrapper.hxx"

namespace dragiyski::node_ext {
    using namespace js;

    /**
     * @brief Template is not a wrapper. The constructor must throw `TypeError("Illegal constructor");`
     * 
     * This is used as a base class in JavaScript for both FunctionTemplate and ObjectTemplate wrappers.
     * 
     * The entire ObjectTemplate/FunctionTemplate must be initialized in the constructor.
     * The constructor will accept the following:
     * 
     * ```javascript
     * new ObjectConstructor({
     *     properties: {
     *         <name/symbol>: <Template.AccessorProperty/Template.NativeDataProperty/Template.LazyDataProperty/ObjectTemplate.Accessor/FunctionTemplate/ObjectTemplate/primitive
     *     }
     * })
     * ```
     * 
     * properties must be either Object or a Map. If Object, only own enumerable properties and symbols are examined.
     * The value must be either primitive or an object instanceof one the classes specified above.
     * 
     * If properties is a Map, keys must be either a string, a Symbol or an Object instanceof Private.
     * Alternatively is the option "private" is a Map, it must contain only keys that are objects instanceof Private.
     * The values for Privates can only be FunctionTemplate/ObjectTemplate or primitive values (including symbols).
     * 
     * The above specification does not allow repetition of keys. Objects cannot have duplicate string key or symbol. Maps cannot specify
     * the same primitive or object twice.
     * 
     * Wrappers of those objects must be stored in a map in the template wrapper, so they can be examined. However, all values will be
     * readonly.
     */
    class Template {
    public:
        static void initialize(v8::Isolate* isolate);
        static void uninitialize(v8::Isolate* isolate);
    public:
        static v8::Local<v8::FunctionTemplate> get_class_template(v8::Isolate* isolate);
        static v8::Local<v8::Private> get_class_symbol(v8::Isolate* isolate);
    private:
        static void constructor(const v8::FunctionCallbackInfo<v8::Value> &info);
    public:
        class AccessorProperty;
        class NativeDataProperty;
        class LazyDataProperty;
    };
}

#endif NODE_EXT_API_TEMPLATE_HXX