export module ppnt.jr.builtin.navigator;

import std;
import v8;
import ppnt.common;

export namespace ppnt::jr::builtin {

    struct NavigatorExtension {

        static auto register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void;
    };

}