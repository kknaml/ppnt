export module ppnt.jr.builtin.console;

import std;
import v8;

export namespace ppnt::jr::builtin {

    struct ConsoleExtension {

        static auto register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void;
    };
}
