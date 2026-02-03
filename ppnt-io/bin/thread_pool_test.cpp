
import ppnt.traits;
import ppnt.io.runtime;
import ppnt.io.task;
import std;
import ppnt.log;
import ppnt.err;
import thread_pool;


using namespace ppnt;

auto async_main() -> io::Task<Unit> {
    log::info({"hahaha"});
    dp::thread_pool pool{2};
    auto s = co_await io::run_in_pool(pool, [] {
        // throw std::runtime_error("www");
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        return 60;
    });
    if (s) {
        log::info({"hahaha result: {}"}, *s);
    } else {
        log::error({"error: {}"}, s.error());
    }

    co_return {};
}

auto main() -> int {

    auto r = io::block_on(async_main());
}