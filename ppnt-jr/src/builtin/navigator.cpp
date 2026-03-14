module ppnt.jr.builtin.navigator;

import std;
import v8;
import ppnt.common;
import ppnt.jr.builtin.helper;
import ppnt.jr.env.br_env;

namespace ppnt::jr::builtin {

    namespace {

        auto navigator_name_property_getter(
            v8::Local<v8::Name> property,
            const v8::PropertyCallbackInfo<v8::Value> &info
        ) -> v8::Intercepted {
            auto *isolate = info.GetIsolate();
            auto context = isolate->GetCurrentContext();

            auto proto = info.HolderV2()->GetPrototypeV2().As<v8::Object>();
            if (!proto->HasRealNamedProperty(context, property).FromMaybe(false)) {
                auto key = v8::String::Utf8Value(isolate, property);
                auto key_str = std::string(*key);
                log::info({"Navigator GET Unknown key: {}"}, key_str);
            }
            return v8::Intercepted::kNo;
        }

        auto get_user_agent_callback(const v8::FunctionCallbackInfo<v8::Value> &info) -> void {
            auto *isolate = info.GetIsolate();
            auto context = isolate->GetCurrentContext();

            auto env_ext = context->GetEmbedderData(0).As<v8::External>();
            auto *env = static_cast<env::BrEnv *>(env_ext->Value());
            if (env) {
                auto &ua_str = env->navigator_data.user_agent;
                if (ua_str.empty()) {
                    log::warn({"navigator.userAgent is empty"});
                }
                auto s = str(isolate, ua_str);
                info.GetReturnValue().Set(s);
            }
        }

        auto init_navigator_proto_tmpl(v8::Isolate *isolate, v8::Local<v8::ObjectTemplate> tmpl) -> void {
            tmpl->SetAccessorProperty(
                str(isolate, "userAgent"),
                v8::FunctionTemplate::New(isolate, get_user_agent_callback),
                v8::Local<v8::FunctionTemplate>{},
                v8::PropertyAttribute::None
            );
        }
    }

    auto NavigatorExtension::register_extension(v8::Isolate *isolate, v8::Local<v8::Object> global) -> void {
        auto context = isolate->GetCurrentContext();
        auto navigator_tmpl = v8::FunctionTemplate::New(isolate, illegal_constructor<"Navigator">);



        auto name = str(isolate, "Navigator");
        navigator_tmpl->SetClassName(name);

        auto proto_tmpl = navigator_tmpl->PrototypeTemplate();
        init_navigator_proto_tmpl(isolate, proto_tmpl);

        // auto navigator_instance = navigator_tmpl->GetFunction(context).ToLocalChecked()->NewInstance(context).ToLocalChecked();

        auto config = v8::NamedPropertyHandlerConfiguration(
             navigator_name_property_getter,
             nullptr,
             nullptr,
             nullptr,
             nullptr,
             v8::Local<v8::Value>{},
             v8::PropertyHandlerFlags::kNone
        );

        auto instance_tmpl = navigator_tmpl->InstanceTemplate();
        instance_tmpl->SetHandler(config);
        auto navigator_instance = instance_tmpl
                                ->NewInstance(context)
                                .ToLocalChecked();

        global->Set(context, str(isolate, "navigator"), navigator_instance).Check();
    }
}
