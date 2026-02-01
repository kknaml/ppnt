
import std;
import ppnt.http.http_types;
import ppnt.io.task;
import ppnt.io;
import ppnt.http.http1;
import ppnt.http.http2;
import ppnt.net.tcp_stream;
import ppnt.io.runtime;
import ppnt.log;
import ppnt.net.tls;
import ppnt.net.ssl;
import ppnt.traits;

using namespace ppnt;

auto async_main() -> io::Task<int> {
    auto s = co_await net::TcpStream::connect("baidu.com", 80);
    if (!s) {
        log::error({"error: {}"}, s.error());
        co_return -1;
    }
    auto request = http::Http1RequestBuilder().build();

    auto session = http::Http1Session(std::move(*s));
    auto http_res = co_await session.request(std::move(request));
    if (!http_res) {
        log::error({"request error: {}"}, http_res.error());
        co_return -1;
    }
    log::info({"http result: \n{}"}, *http_res);
    auto body = co_await http_res->body_full();
    if (!body) {
        log::error({"body error: {}"}, body.error());
        co_return -1;
    }
    auto str = std::string_view(reinterpret_cast<char *>(body->data()), body->size());
    log::info({"body: \n{}"}, str);
    co_return 50;
}

auto async_main2() -> io::Task<Unit> {
    auto stream = co_await net::TcpStream::connect("tls.peet.ws", 443);
    if (!stream) {
        log::error({"stream connect error: {}"}, stream.error());
        co_return {};
    }
    auto ctx = net::TlsContext::client();
    auto tls_res = co_await net::TlsStream::connect(std::move(*stream), std::move(*ctx), "tls.peet.ws");
    if (!tls_res) {
        log::error({"TLS connect failed: {}"}, tls_res.error());
        co_return {};
    }
    auto request_builder = http::Http1RequestBuilder();
    request_builder.path("/api/all");
    auto request = std::move(request_builder).build();
    auto session = http::Http2Session<net::TlsStream>(std::move(*tls_res));
    auto http_res = co_await session.request(std::move(request));
    if (!http_res) {
        log::error({"request error: {}"}, http_res.error());
        co_return {};
    }
    log::info({"http result: \n{}"}, *http_res);
    auto body = co_await http_res->body_full();
    if (!body) {
        log::error({"body error: {}"}, body.error());
        co_return {};
    }
    auto str = std::string_view(reinterpret_cast<char *>(body->data()), body->size());
    log::info({"body: \n{}"}, str);
    co_return {};
}

int main() {

    // auto r = io::block_on(async_main);
    auto r = io::block_on(async_main2);
}