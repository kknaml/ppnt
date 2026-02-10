
export module ppnt.jr.builtin.extension;

import std;
import v8;
import ppnt.common;

export namespace ppnt::jr::builtin {

    template<typename T>
    concept GlobalExtensionTrait = requires(v8::Isolate *isolate) {
        { T::setup_template(isolate) } -> std::same_as<v8::Local<v8::ObjectTemplate>>;
    };

    template <typename T>
    concept V8ExtensionTrait = requires(v8::Isolate *isolate, v8::Local<v8::Object> global) {
        { T::register_extension(isolate, global) } -> std::same_as<void>;
    };

    auto register_all(v8::Isolate *isolate) -> v8::Local<v8::Context> ;
}
