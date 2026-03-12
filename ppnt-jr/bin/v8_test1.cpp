
import v8;
import std;
import ppnt.log;
import ppnt.jr.runtime;

auto main() -> int {
    auto script_path = std::filesystem::path{"./index.js"};
    auto script = std::ifstream{script_path, std::ios::binary};
    if (!script.is_open()) {
        ppnt::log::error({"Failed to open script: {}"}, script_path.string());
        return 1;
    }
    auto code = std::string{
        std::istreambuf_iterator<char>{script},
        std::istreambuf_iterator<char>{}
    };

    auto platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    {
        auto runtime = ppnt::jr::Runtime{};
        runtime.execute_script(code, true);
    }
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    return 0;
}
