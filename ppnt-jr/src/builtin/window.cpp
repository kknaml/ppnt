module ppnt.jr.builtin.window;

import ppnt.jr.builtin.helper;
import ppnt.common;

namespace ppnt::jr::builtin {

    namespace {
        auto window_alert(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            log::info({"window.alert called"});
        }
    }

    auto WindowExtension::setup_template(v8::Isolate *isolate) -> v8::Local<v8::ObjectTemplate> {
        v8::EscapableHandleScope handle_scope(isolate);
        auto window_ctor = v8::FunctionTemplate::New(
            isolate,
            illegal_constructor<"Window">
        );
        set_class_name(isolate, window_ctor, "Window");

        auto proto_tmpl = window_ctor->PrototypeTemplate();
        set_method(isolate, proto_tmpl, "alert", window_alert);

        auto instance_tmpl = window_ctor->InstanceTemplate();
        instance_tmpl->SetInternalFieldCount(1);
        set_val(isolate, instance_tmpl, "Window", window_ctor);

        return handle_scope.Escape(instance_tmpl);
    }

    auto WindowExtension::register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void {
        set_val(isolate, global, "window", global);
        set_val(isolate, global, "globalThis", global);
    }
}
