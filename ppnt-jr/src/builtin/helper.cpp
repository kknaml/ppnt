module ppnt.jr.builtin.helper;

namespace ppnt::jr::builtin {
    auto str(v8::Isolate *isolate, std::string_view data) -> v8::Local<v8::String> {
        return v8::String::NewFromUtf8(isolate, data.data()).ToLocalChecked();
    }

    auto prop_attr(bool writable, bool enumerable, bool configurable) -> v8::PropertyAttribute {
        auto attr = v8::PropertyAttribute::None;
        if (!writable) {
            attr = static_cast<v8::PropertyAttribute>(attr | v8::PropertyAttribute::ReadOnly);
        }
        if (!enumerable) {
            attr = static_cast<v8::PropertyAttribute>(attr | v8::PropertyAttribute::DontEnum);
        }
        if (!configurable) {
            attr = static_cast<v8::PropertyAttribute>(attr | v8::PropertyAttribute::DontDelete);
        }
        return attr;
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

    auto set_val(v8::Isolate *isolate, v8::Local<v8::Object> obj, std::string_view name,
        v8::Local<v8::Value> val, v8::PropertyAttribute attr) -> void {
        obj->DefineOwnProperty(isolate->GetCurrentContext(), str(isolate, name), val, attr).Check();
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
        v8::FunctionCallback callback,
        v8::PropertyAttribute attr
    ) -> void {
        auto func = v8::FunctionTemplate::New(isolate, callback);
        auto v8_name = str(isolate, name);
        func->SetClassName(v8_name);
        tmpl->Set(v8_name, func, attr);
    }

    auto set_method(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::FunctionCallback callback,
        v8::PropertyAttribute attr
    ) -> void {
        auto context = isolate->GetCurrentContext();
        auto v8_name = str(isolate, name);
        auto func = v8::Function::New(context, callback).ToLocalChecked();
        func->SetName(v8_name);
        obj->DefineOwnProperty(context, v8_name, func, attr).Check();
    }

    auto set_proto_method(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::FunctionCallback callback,
        v8::PropertyAttribute attr
    ) -> void {
        set_method(isolate, ctor->PrototypeTemplate(), name, callback, attr);
    }

    auto set_proto_val(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::Local<v8::Value> val,
        v8::PropertyAttribute attr
    ) -> void {
        set_val(isolate, ctor->PrototypeTemplate(), name, val, attr);
    }

    auto set_instance_val(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::Local<v8::Value> val,
        v8::PropertyAttribute attr
    ) -> void {
        set_val(isolate, ctor->InstanceTemplate(), name, val, attr);
    }

    auto define_data_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable,
        bool enumerable,
        bool configurable
    ) -> void {
        set_val(isolate, obj, name, val, prop_attr(writable, enumerable, configurable));
    }

    auto define_method_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::FunctionCallback callback,
        bool writable,
        bool enumerable,
        bool configurable
    ) -> void {
        set_method(isolate, obj, name, callback, prop_attr(writable, enumerable, configurable));
    }

    auto define_proto_data_property(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable,
        bool enumerable,
        bool configurable
    ) -> void {
        set_proto_val(isolate, ctor, name, val, prop_attr(writable, enumerable, configurable));
    }

    auto define_proto_method_property(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::FunctionCallback callback,
        bool writable,
        bool enumerable,
        bool configurable
    ) -> void {
        set_proto_method(isolate, ctor, name, callback, prop_attr(writable, enumerable, configurable));
    }

    auto define_global_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> global,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable,
        bool enumerable,
        bool configurable
    ) -> void {
        define_data_property(isolate, global, name, val, writable, enumerable, configurable);
    }

    auto define_unforgeable_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable,
        bool enumerable
    ) -> void {
        define_data_property(isolate, obj, name, val, writable, enumerable, false);
    }

    auto set_proto(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        v8::Local<v8::Value> proto
    ) -> void {
        obj->SetPrototypeV2(isolate->GetCurrentContext(), proto).Check();
    }

    auto get_proto(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj
    ) -> v8::Local<v8::Value> {
        return obj->GetPrototypeV2().As<v8::Value>();
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
