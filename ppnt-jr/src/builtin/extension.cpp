module ppnt.jr.builtin.extension;

import ppnt.jr.builtin.window;
import ppnt.jr.builtin.console;
import ppnt.jr.builtin.timeout;
import ppnt.jr.builtin.fetch;
import ppnt.jr.builtin.xhr;

namespace ppnt::jr::builtin {

    template<GlobalExtensionTrait GlobalExt, V8ExtensionTrait ...Exts>
    auto create(v8::Isolate *isolate) -> v8::Local<v8::Context> {
        auto handle_scope = v8::EscapableHandleScope{isolate};
        // auto global_tmpl = v8::ObjectTemplate::New(isolate);
        auto global_tmpl = GlobalExt::setup_template(isolate);

        auto context = v8::Context::New(isolate, nullptr, global_tmpl);
        auto context_scope = v8::Context::Scope{context};
        auto global_obj = context->Global();

        if constexpr (requires { GlobalExt::register_extension(isolate, global_obj);}) {
            GlobalExt::register_extension(isolate, global_obj);
        }

        (Exts::register_extension(isolate, global_obj), ...);
        return handle_scope.Escape(context);
    }

    auto register_all(v8::Isolate *isolate) ->v8::Local<v8::Context> {
      return create<
          WindowExtension,
          ConsoleExtension,
          TimeoutExtension,
          FetchExtension,
          XMLHttpRequestExtension
      >(isolate);
    }
}
