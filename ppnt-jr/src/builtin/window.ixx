export module ppnt.jr.builtin.window;

import std;
import v8;

export namespace ppnt::jr::builtin {

    struct WindowExtension {

        static auto setup_template(v8::Isolate *isolate) -> v8::Local<v8::ObjectTemplate>;

        static auto register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void;
    };
}