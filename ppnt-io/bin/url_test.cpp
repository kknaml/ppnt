
import std;
import ppnt.net.url;
import ppnt.common;

using namespace ppnt::net;

auto main() -> int {
    auto u1 = Url::parse("https://user:pass@Example.COM:8080/a/./b/../../c?q=1#frag");
    if (!u1) {
        ppnt::log::error({"{}"}, u1.error());
    }
    ppnt::log::info({"{}"}, u1->scheme());
    ppnt::log::info({"{}"}, u1->host());
    ppnt::log::info({"{}"}, u1->port_or_default());
}