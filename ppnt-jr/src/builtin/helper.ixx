
export module ppnt.jr.builtin.helper;

import std;
import v8;
import ppnt.common;


export namespace ppnt::jr::builtin {

    auto str(v8::Isolate *isolate, std::string_view data) -> v8::Local<v8::String>;
    auto prop_attr(bool writable = true, bool enumerable = true, bool configurable = true) -> v8::PropertyAttribute;

    template<FixedString ClassName>
    auto illegal_constructor(const v8::FunctionCallbackInfo<v8::Value> &args) {
        auto *isolate = args.GetIsolate();
        auto handle_scope = v8::HandleScope{isolate};
        if (args.IsConstructCall()) {
            auto msg = std::format("Failed to construct '{}': Illegal constructor", ClassName.c_str());
            isolate->ThrowException(v8::Exception::TypeError(str(isolate, msg)));
        } else {
            auto msg = "Illegal constructor";
            isolate->ThrowException(v8::Exception::TypeError(str(isolate, msg)));
        }
    }

    auto set_val(
        v8::Isolate *isolate,
        v8::Local<v8::Template> tmpl,
        std::string_view name,
        v8::Local<v8::Value> val,
        v8::PropertyAttribute attr = v8::PropertyAttribute::None
    ) -> void;

    auto set_val(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::Local<v8::Value> val
    ) -> void;
    auto set_val(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::Local<v8::Value> val,
        v8::PropertyAttribute attr
    ) -> void;

    auto set_val(
        v8::Isolate *isolate,
        v8::Local<v8::Template> tmpl,
        std::string_view name,
        v8::Local<v8::Data> val,
        v8::PropertyAttribute attr = v8::PropertyAttribute::None
    ) -> void;

    auto set_accessor(
        v8::Isolate *isolate,
        v8::Local<v8::Template> tmpl,
        std::string_view name,
        v8::AccessorNameGetterCallback getter,
        v8::AccessorNameSetterCallback setter = nullptr,
        v8::PropertyAttribute attr = v8::PropertyAttribute::None
    ) -> void;

    auto set_method(
        v8::Isolate *isolate,
        v8::Local<v8::Template> tmpl,
        std::string_view name,
        v8::FunctionCallback callback,
        v8::PropertyAttribute attr = v8::PropertyAttribute::DontEnum
    ) -> void;

    auto set_method(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::FunctionCallback callback,
        v8::PropertyAttribute attr = v8::PropertyAttribute::DontEnum
    ) -> void;

    auto set_proto_method(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::FunctionCallback callback,
        v8::PropertyAttribute attr = v8::PropertyAttribute::DontEnum
    ) -> void;

    auto set_proto_val(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::Local<v8::Value> val,
        v8::PropertyAttribute attr = v8::PropertyAttribute::None
    ) -> void;

    auto set_instance_val(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::Local<v8::Value> val,
        v8::PropertyAttribute attr = v8::PropertyAttribute::None
    ) -> void;

    auto define_data_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable = true,
        bool enumerable = true,
        bool configurable = true
    ) -> void;

    auto define_method_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::FunctionCallback callback,
        bool writable = true,
        bool enumerable = false,
        bool configurable = true
    ) -> void;

    auto define_proto_data_property(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable = true,
        bool enumerable = true,
        bool configurable = true
    ) -> void;

    auto define_proto_method_property(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name,
        v8::FunctionCallback callback,
        bool writable = true,
        bool enumerable = false,
        bool configurable = true
    ) -> void;

    auto define_global_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> global,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable = true,
        bool enumerable = false,
        bool configurable = true
    ) -> void;

    auto define_unforgeable_property(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        std::string_view name,
        v8::Local<v8::Value> val,
        bool writable = true,
        bool enumerable = true
    ) -> void;

    auto set_proto(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj,
        v8::Local<v8::Value> proto
    ) -> void;

    auto get_proto(
        v8::Isolate *isolate,
        v8::Local<v8::Object> obj
    ) -> v8::Local<v8::Value>;

    auto get_to_string_tag(
        v8::Isolate *isolate
    ) -> v8::Local<v8::Symbol>;

    auto set_class_name(
        v8::Isolate *isolate,
        v8::Local<v8::FunctionTemplate> ctor,
        std::string_view name
    ) -> void;

    auto set_interceptor(
        v8::Local<v8::ObjectTemplate> tmpl,
        v8::NamedPropertyGetterCallback getter,
        v8::NamedPropertySetterCallback setter = nullptr
    ) -> void;
}
