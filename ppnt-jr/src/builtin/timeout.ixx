export module ppnt.jr.builtin.timeout;

import v8;

export namespace ppnt::jr::builtin {

    struct TimeoutExtension {
        static auto register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void;
    };
}
