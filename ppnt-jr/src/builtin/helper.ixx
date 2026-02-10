
export module ppnt.jr.builtin.helper;

import std;
import v8;
import ppnt.common;


export namespace ppnt::jr::builtin {

    auto str(v8::Isolate *isolate, std::string_view data) -> v8::Local<v8::String>;

    template<FixedString ClassName>
    auto illegal_constructor(const v8::FunctionCallbackInfo<v8::Value> &args) {
        auto *isolate = args.GetIsolate();
        auto handle_scope = v8::HandleScope{isolate};
        auto msg = std::format("Failed to construct '{}': Illegal constructor", ClassName.c_str());
        isolate->ThrowException(v8::Exception::TypeError(str(isolate, msg)));
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
        v8::FunctionCallback callback
    ) -> void;

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
