
import v8;
import std;
import ppnt.log;
import ppnt.jr.builtin;

auto main() -> int {
    auto platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    auto *isolate = v8::Isolate::New(create_params);
    {
        auto isolate_scope = v8::Isolate::Scope(isolate);
        auto handle_scope = v8::HandleScope(isolate);
        // auto context = v8::Context::New(isolate);
        auto context = ppnt::jr::builtin::register_all(isolate);
        auto context_scope = v8::Context::Scope(context);
        auto source = v8::String::NewFromUtf8(
            isolate,
            "(() => Window.prototype.alert)()",
            v8::NewStringType::kNormal).ToLocalChecked();
        auto script = v8::Script::Compile(context, source);
        if (script.IsEmpty()) {
            ppnt::log::error({"Script compile failed"});
        } else {
            auto result = script.ToLocalChecked()->Run(context);
            if (!result.IsEmpty()) {
                auto utf8 = v8::String::Utf8Value(isolate, result.ToLocalChecked());
                ppnt::log::info({"result: {}"}, *utf8);
            } else {
                ppnt::log::error({"Result is empty"});
            }
        }


    }
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    delete create_params.array_buffer_allocator;
}

