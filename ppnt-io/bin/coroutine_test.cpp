import std;
import ppnt.common;
import ppnt.io;
using namespace ppnt;

auto async_main() -> io::Task<Unit> {
    log::info({"async main start: {}"}, std::this_thread::get_id());
    co_await io::delay(2000);
    log::info({"2000 ms"});
    auto r = co_await io::suspend_coroutine<int>([] (auto cont) {
        log::info({"suspend_coroutine inner: {}"}, std::this_thread::get_id());
        std::thread([cont] {
            log::info({"thread enter: {}"}, std::this_thread::get_id());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            log::info({"thread sleep 1000: {}"}, std::this_thread::get_id());
            cont(60);
        }).detach();
    });
    log::info({"resume from suspend: {} : {}"}, r, std::this_thread::get_id());
    log::info({"async main end: {}"}, std::this_thread::get_id());
    co_return {};
}

auto main() -> int {
    auto r = io::block_on(async_main());
}