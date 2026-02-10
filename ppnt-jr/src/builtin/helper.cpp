module ppnt.jr.builtin.helper;

namespace ppnt::jr::builtin {
    auto str(v8::Isolate *isolate, std::string_view data) -> v8::Local<v8::String> {
        return v8::String::NewFromUtf8(isolate, data.data()).ToLocalChecked();
    }

    auto set_val(
        v8::Isolate *isolate,
        v8::Local<v8::Template> tmpl,
        std::string_view name,
        v8::Local<v8::Value> val,
        v8::PropertyAttribute attr
    ) -> void {
        tmpl->Set(str(isolate, name), val, attr);
    }

    auto set_val(v8::Isolate *isolate, v8::Local<v8::Object> obj, std::string_view name,
        v8::Local<v8::Value> val) -> void {
        obj->Set(isolate->GetCurrentContext(), str(isolate, name), val).Check();
    }

    auto set_val(v8::Isolate *isolate, v8::Local<v8::Template> tmpl, std::string_view name,
        v8::Local<v8::Data> val, v8::PropertyAttribute attr) -> void {
        v8::HandleScope scope(isolate);
        auto name_str = v8::String::NewFromUtf8(isolate, name.data(), v8::NewStringType::kNormal, name.length()).ToLocalChecked();
        tmpl->Set(name_str, val, attr);
    }


    auto set_accessor(
        v8::Isolate *isolate,
        v8::Local<v8::Template> tmpl,
        std::string_view name,
        v8::AccessorNameGetterCallback getter,
        v8::AccessorNameSetterCallback setter,
        v8::PropertyAttribute attr
    ) -> void {
        tmpl->SetNativeDataProperty(str(isolate, name), getter, setter, {}, attr);
    }

    auto set_method(
        v8::Isolate *isolate,
        v8::Local<v8::Template> tmpl,
        std::string_view name,
        v8::FunctionCallback callback
    ) -> void {
        auto func = v8::FunctionTemplate::New(isolate, callback);
        auto v8_name = str(isolate, name);
        func->SetClassName(v8_name);
        tmpl->Set(v8_name, func);
    }

    auto get_to_string_tag(v8::Isolate *isolate) -> v8::Local<v8::Symbol> {
        return v8::Symbol::GetToStringTag(isolate);
    }

    auto set_class_name(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name
    ) -> void {
        auto v8_name = str(isolate, name);
        ctor->SetClassName(v8_name);
        auto prop = ctor->PrototypeTemplate();
        prop->Set(get_to_string_tag(isolate), v8_name, v8::PropertyAttribute::DontEnum);
    }

    auto set_interceptor(
        v8::Local<v8::ObjectTemplate> tmpl,
        v8::NamedPropertyGetterCallback getter,
        v8::NamedPropertySetterCallback setter
    ) -> void {
        auto config = v8::NamedPropertyHandlerConfiguration{
            getter, setter, nullptr, nullptr, nullptr, v8::Local<v8::Value>{}, v8::PropertyHandlerFlags::kNonMasking
        };
        tmpl->SetHandler(config);
    }
}
