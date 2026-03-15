
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

    auto build_test_request() -> HttpRequest {
        auto request = HttpRequest{};
        auto url = net::Url::parse("http://example.com/");
        request.method = Method::GET;
        request.url = *url;
        request.headers.add("Connection", "keep-alive");
        return request;
    }

    auto log_response_meta(int index, const HttpResponse<AnySession> &resp, const void *session_ptr) -> void {
        auto connection_header = resp.get_headers().get("connection").value_or("<missing>");
        auto keep_alive_header = resp.get_headers().get("keep-alive").value_or("<missing>");
        log::info({
            "[{}] status={} version={} session={} connection={} keep-alive={}"
        },
            index,
            resp.get_status().code,
            std::to_underlying(resp.get_session()->get_http_version()),
            session_ptr,
            connection_header,
            keep_alive_header
        );
    }

    auto async_main() -> io::Task<> {
        HttpClient client{};
        const void *last_session_ptr = nullptr;

        for (int i = 1; i <= kRequestCount; ++i) {
            auto request = build_test_request();
            auto res = co_await client.request(std::move(request));
            if (!res) {
                log::error({"[{}] request error: {}"}, i, res.error());
                co_return;
            }

            auto session_ptr = static_cast<const void *>(res->get_session());
            log_response_meta(i, *res, session_ptr);

            if (last_session_ptr) {
                log::info({"[{}] keep-alive reused_session={}"}, i, session_ptr == last_session_ptr);
            }
            last_session_ptr = session_ptr;

            auto body = co_await res->body_full();
            if (!body) {
                log::error({"[{}] body drain error: {}"}, i, body.error());
                co_return;
            }

            log::info({"[{}] drained_body_bytes={}"}, i, body->size());
        }
    }
}

auto main() -> int {
    io::block_on(async_main());
    return 0;
}
