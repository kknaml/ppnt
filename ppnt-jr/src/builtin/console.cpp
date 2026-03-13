module ppnt.jr.builtin.console;

import ppnt.jr.builtin.helper;
import ppnt.common;

namespace ppnt::jr::builtin {

    namespace {
        auto stringify_args(v8::Isolate *isolate, const v8::FunctionCallbackInfo<v8::Value> &info, std::string_view level) -> std::string {
            auto context = isolate->GetCurrentContext();
            std::string line{level};
            line.push_back(' ');

            for (int i = 0; i < info.Length(); ++i) {
                if (i > 0) {
                    line.append(" ");
                }
                v8::String::Utf8Value text(isolate, info[i]->ToString(context).ToLocalChecked());
                line.append(*text != nullptr ? *text : "<non-utf8>");
            }

            return line;
        }

        auto console_debug(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto line = stringify_args(info.GetIsolate(), info, "[debug]");
            log::debug({"[JS DEBUG]: {}"}, line);
        }

        auto console_log(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto line = stringify_args(info.GetIsolate(), info, "[log]");
            log::debug({"[JS LOG]: {}"}, line);
        }

        auto console_info(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto line = stringify_args(info.GetIsolate(), info, "[info]");
            log::info({"[JS INFO]: {}"}, line);
        }

        auto console_warn(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto line = stringify_args(info.GetIsolate(), info, "[warn]");
            log::warn({"[JS WARN]: {}"}, line);
        }

        auto console_error(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto line = stringify_args(info.GetIsolate(), info, "[error]");
            log::error({"[JS ERROR]: {}"}, line);
        }

        auto setup_template(v8::Isolate *isolate) -> v8::Local<v8::ObjectTemplate> {
            auto handle_scope = v8::EscapableHandleScope{isolate};
            auto console_ctor = v8::FunctionTemplate::New(
                isolate,
                illegal_constructor<"Console">
            );
            set_class_name(isolate, console_ctor, "Console");

            auto proto_tmpl = console_ctor->PrototypeTemplate();
            proto_tmpl->Set(
                str(isolate, "log"),
                v8::FunctionTemplate::New(isolate, console_log),
                v8::PropertyAttribute::DontEnum
            );
            proto_tmpl->Set(
                str(isolate, "debug"),
                v8::FunctionTemplate::New(isolate, console_debug),
                v8::PropertyAttribute::DontEnum
            );
            proto_tmpl->Set(
                str(isolate, "info"),
                v8::FunctionTemplate::New(isolate, console_info),
                v8::PropertyAttribute::DontEnum
            );
            proto_tmpl->Set(
                str(isolate, "warn"),
                v8::FunctionTemplate::New(isolate, console_warn),
                v8::PropertyAttribute::DontEnum
            );
            proto_tmpl->Set(
                str(isolate, "error"),
                v8::FunctionTemplate::New(isolate, console_error),
                v8::PropertyAttribute::DontEnum
            );

            auto instance_tmpl = console_ctor->InstanceTemplate();
            instance_tmpl->SetInternalFieldCount(1);
            return handle_scope.Escape(instance_tmpl);
        }
    }

    auto ConsoleExtension::register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void {
        auto context = isolate->GetCurrentContext();
        auto console_tmpl = setup_template(isolate);
        auto console_obj = console_tmpl->NewInstance(context).ToLocalChecked();
        global->DefineOwnProperty(
            context,
            str(isolate, "console"),
            console_obj,
            v8::PropertyAttribute::DontEnum
        ).Check();
    }
}
