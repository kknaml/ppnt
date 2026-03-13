module ppnt.jr.builtin.fetch;

import ppnt.jr.builtin.helper;
import ppnt.common;

namespace ppnt::jr::builtin {

    namespace {

        auto print_fetch_param(const v8::FunctionCallbackInfo<v8::Value> &args) -> void {
            auto isolate = args.GetIsolate();
            auto context = isolate->GetCurrentContext();

            auto msg = std::format("fetch called with {} params", args.Length());
            for (int i = 0;i < args.Length(); i++) {
                auto arg = args[i];
                auto v8_str = v8::Local<v8::String>{};
                if (arg->ToString(context).ToLocal(&v8_str)) {
                    auto utf8_value = v8::String::Utf8Value(isolate, v8_str);
                    msg = std::format("{}\n param[{}] = {}", msg, i, *utf8_value);
                } else {
                    msg = std::format("{}\n param[{}] = ??", msg, i);
                }
            }
            log::info({"{}"}, msg);
        }

        auto fetch_callback(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto isolate = info.GetIsolate();
            auto context = isolate->GetCurrentContext();

            if (info.IsConstructCall()) { // new fetch
                auto err_msg = str(isolate, "fetch is not a constructor");
                isolate->ThrowException(v8::Exception::TypeError(err_msg));
                return;
            }

            print_fetch_param(info);
            // TODO mock fetch

            auto loc = std::source_location::current();
            auto msg = std::format("TODO fetch at {}:{}" ,loc.file_name(), loc.line());
            isolate->ThrowException(v8::Exception::Error(str(isolate, msg)));
        }
    }

    auto FetchExtension::register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void {
        auto context = isolate->GetCurrentContext();

        auto fetch_tmpl = v8::FunctionTemplate::New(isolate, fetch_callback);
        auto fetch_fn = fetch_tmpl->GetFunction(context).ToLocalChecked();
        auto name = str(isolate, "fetch");
        fetch_fn->SetName(name);
        global->Set(context, name, fetch_fn).Check();
    }
}
