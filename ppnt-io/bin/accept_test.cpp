
import std;
import ppnt.common;
import ppnt.io;
import ppnt.io.task;
import ppnt.net.accept;
import ppnt.net.addr;
import ppnt.net.tcp_stream;

using namespace ppnt;
using namespace ppnt::net;

auto accept_handler(TcpStream stream) -> io::Task<> {
    log::info({"Accept: {}"}, stream.native_handle());
    auto r = co_await stream.read_bs(4096);
    if (r) {
        auto str = std::string_view{
            static_cast<char *>(r.buffer),
            static_cast<uint64_t>(r.result)
        };
        log::info({"read {} data,Accept: {}, dasdad"}, r.result, str);
    } else {
        log::error({"read failed: {}"}, r.error().error());
    }
    co_return;
}

auto foo() -> io::Task<> {
    auto addr = SocketAddress::from_ip_port("127.0.0.1", 4399);
    if (!addr) {
        log::error({"Invalid address: {}"}, addr.error());
        co_return;
    }
    log::info({"foo: {}"}, *addr);
    co_await run_accept_loop(accept_handler, *addr);
    co_return;
}

auto main() -> int {

    io::block_on(foo());
}