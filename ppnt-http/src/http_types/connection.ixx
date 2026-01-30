export module ppnt.http.http_types:connection;

import std;
import ppnt.traits;
import ppnt.util;
import ppnt.io.task;
import ppnt.err;

export namespace ppnt::http {


    template<typename C>
    concept Connection = requires(C &c, std::span<uint8_t> buf, std::span<const uint8_t> cbuf) {
        { c.read(buf) } -> Awaiterble<Result<int>, io::Task<>::promise_type>;
        { c.write(cbuf) } -> Awaiterble<Result<int>, io::Task<>::promise_type>;
        { c.is_alive() } -> std::convertible_to<bool>;
        { c.close() };
    };
}
