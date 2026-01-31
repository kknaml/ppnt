
import std;
import ppnt.http.http_types;
import ppnt.io.task;
import ppnt.io;
import ppnt.http.http1;
import ppnt.net.tcp_stream;
import ppnt.io.runtime;
import ppnt.log;

using namespace ppnt;

auto async_main() -> io::Task<int> {
    auto s = co_await net::TcpStream::connect("baidu.com", 80);
    if (!s) {
        log::error({"error: {}"}, s.error());
        co_return -1;
    }
    auto request = http::Http1RequestBuilder().build();

    auto session = http::Http1Session(std::move(*s));

    co_return 50;
}

int main() {

    auto r = io::block_on(async_main);
}