
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
    constexpr auto kRequestCount = 5;

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

    auto log_response_meta(
        std::string_view label,
        int index,
        const HttpResponse<AnySession> &resp,
        const void *session_ptr,
        bool reused
    ) -> void {
        auto connection_header = resp.get_headers().get("connection").value_or("<missing>");
        auto keep_alive_header = resp.get_headers().get("keep-alive").value_or("<missing>");
        log::info(
            {"[{}:{}] status={} version={} session={} reused={} connection={} keep-alive={}"},
            label,
            index,
            resp.get_status().code,
            std::to_underlying(resp.get_session()->get_http_version()),
            session_ptr,
            reused,
            connection_header,
            keep_alive_header
        );
    }

    auto run_sequence(std::string_view label, std::string_view url_str, bool add_keep_alive_header) -> io::Task<> {
        HttpClient client{};
        const void *last_session_ptr = nullptr;

        log::info({"[{}] target={} requests={} keep_alive_header={}"},
            label,
            url_str,
            kRequestCount,
            add_keep_alive_header
        );

        for (int i = 1; i <= kRequestCount; ++i) {
            auto req = make_request(url_str, add_keep_alive_header);
            auto res = co_await client.request(std::move(req));
            if (!res) {
                log::error({"[{}:{}] request error: {}"}, label, i, res.error());
                co_return;
            }

            auto session_ptr = static_cast<const void *>(res->get_session());
            auto reused = last_session_ptr && session_ptr == last_session_ptr;
            log_response_meta(label, i, *res, session_ptr, reused);
            last_session_ptr = session_ptr;

            auto body = co_await res->body_full();
            if (!body) {
                log::error({"[{}:{}] body drain error: {}"}, label, i, body.error());
                co_return;
            }

            log::info({"[{}:{}] drained_body_bytes={}"}, label, i, body->size());
        }
    }

    auto async_main() -> io::Task<> {
        co_await run_sequence("h1", "http://example.com/", true);
        co_await run_sequence("h2", "https://nghttp2.org/httpbin/get", false);
    }
}

auto main() -> int {
    io::block_on(async_main());
    return 0;
}
