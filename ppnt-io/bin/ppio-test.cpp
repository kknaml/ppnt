
import std;
import ppnt.io.task;
import ppnt.log;
import ppnt.libc;
import ppnt.net;
import ppnt.match;
import ppnt.err;
import ppnt.net.tcp_stream;
import ppnt.io.runtime;
import ppnt.traits;
import ppnt.net.ssl;
import ppnt.io;

using namespace ppnt;

auto bar() -> io::Task<long> {

    co_return 60;
}

auto foo() -> io::Task<int> {
    log::info({"foo start"});
    auto x = co_await bar();
    co_return 20;
}

auto async_main() -> io::Task<int> {

    auto result = net::resolve_first("www.baidu.com", 80);

    if (result) {
        auto &addrs = *result;
        log::info({"addr: {}"}, addrs);
        auto s = co_await net::TcpStream::connect(addrs);
        if (s) {
            log::info({"connected: "});
            auto &tcp_stream = *s;
            std::string_view req = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n";
            auto write_res = co_await tcp_stream.write_all(std::span{(uint8_t *)req.data(), req.length()});
            if (write_res) {
                log::info({"write {} bytes"}, req.size());
            } else {
                log::error({"write failed: {}"}, write_res.error());
                co_return 1;
            }
            uint8_t buf[4096];
            while (true) {
                auto read_res = co_await tcp_stream.read(std::span{buf});
                if (!read_res) {
                    log::error({"read failed"}, read_res.error());
                    break;
                }
                auto n = *read_res;
                if (n == 0) {
                    log::info({"io end"});
                    break;
                }
                log::info({"read {}"}, std::string_view{(char *)buf, n});
            }
        } else {
            log::error({"Error: {}"}, s.error());
            co_return 2;
        }
    } else {
        log::error({"Resolution failed: {}"}, result.error());
    }

    co_return 30;
}

auto https_demo() -> io::Task<int> {
    auto stream = co_await net::TcpStream::connect("tls.peet.ws", 443);
    auto ctx = net::TlsContext::client();
    auto tls_res = co_await net::TlsStream::connect(std::move(*stream), std::move(*ctx), "tls.peet.ws");
    if (!tls_res) {
        log::error({"TLS connect failed: {}"}, tls_res.error());
        co_return 10;
    }
    auto tls = std::move(*tls_res);
    std::string req = "GET /api/all HTTP/1.1\r\nHost: tls.peet.ws\r\nConnection: close\r\n\r\n";
    auto r = co_await tls.write(std::span{(uint8_t *)req.c_str(), req.size()});
    if (!r) {
        log::error({"write failed"}, r.error());
        co_return 30;
    }
    log::info({"write {}"}, *r);
    uint8_t buf[1024];
    auto body = io::BinartWriter{};
    while (true) {
        auto res = co_await tls.read(std::span{buf});
        if (!res) {
            log::error({"tls read: {}"}, res.error());
            break;
        }
        if (*res == 0) break;
        log::info({"read {}"}, *res);
        body.write_bytes(buf, *res);
    }
    auto str = std::string_view{reinterpret_cast<char *>(body.buffer().data()), body.size()};
    log::info({"msg:\n {}"}, str);
    co_return 50;
}

auto main() -> int {

    // auto result = io::block_on(async_main());
    auto result = io::block_on(https_demo());
    log::info({"main end with {}"}, result);
}
