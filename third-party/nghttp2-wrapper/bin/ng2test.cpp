import nghttp2;
import std;
import ppnt.log;

using namespace nghttp2;

auto main(int argc, char **argv) -> int {
    auto *version = nghttp2_version(0);
    ppnt::log::info({"age: {}, proto: {}, versionNum: {}, version_str: {}"},
        version->age,
        version->proto_str,
        version->version_num,
        version->version_str
    );
    std::println(std::cerr, "huahuahua");
}
