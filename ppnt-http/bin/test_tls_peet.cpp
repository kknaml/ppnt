
import std;
import ppnt.http.client;
import ppnt.http.http_types;
import ppnt.io.runtime;
import ppnt.io.task;
import ppnt.log;
import ppnt.net.tls;
import ppnt.net.url;
import ppnt.common;

using namespace ppnt;

namespace {
    auto make_request() -> http::HttpRequest {
        auto req = http::HttpRequest{};
        auto url = net::Url::parse("https://tls.peet.ws/api/all");
        req.method = http::Method::GET;
        req.url = *url;
        return req;
    }

    auto dump_body(std::string_view label, std::span<const uint8_t> body) -> void {
        auto text = std::string_view{
            reinterpret_cast<const char *>(body.data()),
            body.size()
        };
        log::info({"[{}] tls.peet response:\n{}"}, label, text);
    }

    auto run_once(std::string_view label, net::ClientHelloSpecFactory *spec_factory = nullptr) -> io::Task<> {
        auto client = http::HttpClient({
            .tls_spec_factory = spec_factory,
        });
        auto res = co_await client.request(make_request());
        if (!res) {
            log::error({"[{}] request error: {}"}, label, res.error());
            co_return;
        }
        auto body = co_await res->body_full();
        if (!body) {
            log::error({"[{}] body error: {}"}, label, body.error());
            co_return;
        }
        dump_body(label, *body);
        auto close_res = co_await client.close_async();
        if (!close_res) {
            log::error({"[{}] client close error: {}"}, label, close_res.error());
        }
        co_return;
    }

    auto async_main() -> io::Task<> {
        std::unique_ptr<net::ClientHelloSpecFactory> spec_factory{
            net::create_hello_spec_factory(net::make_test_spec)
        };
        spec_factory = nullptr;
        co_await run_once("default");
        co_await run_once("patched", spec_factory.get());
        co_return;
    }
}

auto main() -> int {
    io::block_on(async_main());
    return 0;
}
