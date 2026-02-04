export module ppnt.io.stream;

import std;
import ppnt.net.tcp_stream;
import ppnt.net.ssl;
import ppnt.net.tls;
import ppnt.io.task;
import ppnt.err;
import ppnt.traits;

namespace ppnt::io {

    export using AnyStream = std::variant<
        net::TcpStream,
        net::TlsStream
    >;

    export auto stream_read(AnyStream &stream, std::span<uint8_t> buf) -> decltype(auto) {
        return std::visit([&](auto &inner) -> Task<Result<int>> {
            co_return co_await inner.read(buf);
        }, stream);
    }
}