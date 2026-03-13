module ppnt.jr.builtin.xhr;

import ppnt.common;
import ppnt.jr.builtin.helper;

namespace ppnt::jr::builtin {

    namespace {
        auto xhr_constructor_callback(const v8::FunctionCallbackInfo<v8::Value> &args) -> void {
            auto isolate = args.GetIsolate();
            if (!args.IsConstructCall()) {
                auto err_msg = str(isolate, "Failed to construct 'XMLHttpRequest': Please use the 'new' operator, this DOM object constructor cannot be called as a function.");
                isolate->ThrowException(v8::Exception::TypeError(err_msg));
                return;
            }

            // TODO set obj
            args.GetReturnValue().Set(args.This());
        }

        auto xhr_open_callback(const v8::FunctionCallbackInfo<v8::Value> &args) -> void {
            auto isolate = args.GetIsolate();
            if (args.Length() < 2) {
                isolate->ThrowException(v8::Exception::TypeError(str(isolate, "Failed to execute 'open' on 'XMLHttpRequest': 2 arguments required")));
                return;
            }
            log::info({"{}"}, std::format("XHR.open called with {} params", args.Length()));
        }

        auto xhr_send_callback(const v8::FunctionCallbackInfo<v8::Value> &args) -> void {
            auto isolate = args.GetIsolate();
            log::info({"{}"}, "XHR.send called");

            // TODO
        }
    }

    auto XMLHttpRequestExtension::register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void {
        auto context = isolate->GetCurrentContext();

        auto xhr_tmpl = v8::FunctionTemplate::New(isolate, xhr_constructor_callback);

        auto name = str(isolate, "XMLHttpRequest");
        xhr_tmpl->SetClassName(name);
        xhr_tmpl->InstanceTemplate()->SetInternalFieldCount(1);

        auto prototype_tmpl = xhr_tmpl->PrototypeTemplate();
        prototype_tmpl->Set(isolate, "open", v8::FunctionTemplate::New(isolate, xhr_open_callback));
        prototype_tmpl->Set(isolate, "send", v8::FunctionTemplate::New(isolate, xhr_send_callback));

        auto xhr_fn = xhr_tmpl->GetFunction(context).ToLocalChecked();
        global->Set(context, name, xhr_fn).Check();
    }

}
