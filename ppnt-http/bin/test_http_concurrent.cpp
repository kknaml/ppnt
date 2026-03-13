
import std;
import ppnt.http.client;
import ppnt.http.http_types;
import ppnt.io.runtime;
import ppnt.io.task;
import ppnt.log;
import ppnt.net.url;
import ppnt.http.session;

using namespace ppnt;
using namespace ppnt::http;

namespace {
    constexpr auto kConcurrentRequests = 5;

    auto make_request(std::string_view url_str, bool add_keep_alive_header) -> HttpRequest {
        auto request = HttpRequest{};
        auto url = net::Url::parse(url_str);
        request.method = Method::GET;
        request.url = *url;
        if (add_keep_alive_header) {
            request.headers.add("Connection", "keep-alive");
        }
        return request;
    }

    auto run_one(HttpClient &client, std::string_view label, int index, std::string_view url_str, bool add_keep_alive_header) -> io::Task<> {
        auto req = make_request(url_str, add_keep_alive_header);
        auto res = co_await client.request(std::move(req));
        if (!res) {
            log::error({"[{}:{}] request error: {}"}, label, index, res.error());
            co_return;
        }

        auto connection_header = res->get_headers().get("connection").value_or("<missing>");
        auto keep_alive_header = res->get_headers().get("keep-alive").value_or("<missing>");
        auto session_ptr = static_cast<const void *>(res->get_session());
        log::info(
            {"[{}:{}] status={} version={} session={} connection={} keep-alive={}"},
            label,
            index,
            res->get_status().code,
            std::to_underlying(res->get_session()->get_http_version()),
            session_ptr,
            connection_header,
            keep_alive_header
        );

        auto body = co_await res->body_full();
        if (!body) {
            log::error({"[{}:{}] body drain error: {}"}, label, index, body.error());
            co_return;
        }

        log::info({"[{}:{}] drained_body_bytes={}"}, label, index, body->size());
        co_return;
    }

    auto run_concurrent(std::string_view label, std::string_view url_str, bool add_keep_alive_header) -> io::Task<> {
        HttpClient client{};
        std::vector<io::JoinHandle<void>> handles{};
        handles.reserve(kConcurrentRequests);

        log::info({"[{}] target={} concurrent_requests={} keep_alive_header={}"},
            label,
            url_str,
            kConcurrentRequests,
            add_keep_alive_header
        );

        for (int i = 1; i <= kConcurrentRequests; ++i) {
            handles.push_back(io::spawn(run_one(client, label, i, url_str, add_keep_alive_header)));
        }

        for (auto &handle : handles) {
            try {
                co_await std::move(handle);
            } catch (const std::exception &e) {
                log::error({"[{}] spawned task exception: {}"}, label, e.what());
            } catch (...) {
                log::error({"[{}] spawned task exception: <unknown>"}, label);
            }
        }
        co_return;
    }

    auto async_main() -> io::Task<> {
        co_await run_concurrent("h1-concurrent", "http://example.com/", true);
        co_await run_concurrent("h2-concurrent", "https://nghttp2.org/httpbin/get", false);
        co_return;
    }
}

auto main() -> int {
    io::block_on(async_main());
    return 0;
}
