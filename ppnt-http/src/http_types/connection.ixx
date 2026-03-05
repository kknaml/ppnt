export module ppnt.http.http_types:connection;

import std;
import ppnt.common;
import ppnt.io.task;

export namespace ppnt::http {


    template<typename C>
    concept Connection = requires(C &c, std::span<uint8_t> buf, std::span<const uint8_t> cbuf, uint32_t nbytes) {
        // { c.read(buf) } -> Awaiterble<Result<int>, io::Task<>::promise_type>;
        // // { c.read(nbytes) } -> Awaiterble<std::span<uint8_t>, io::Task<>::promise_type>;
        // { c.write(cbuf) } -> Awaiterble<Result<int>, io::Task<>::promise_type>;
        // { c.is_alive() } -> std::convertible_to<bool>;
        { c.close() };
    };
}
