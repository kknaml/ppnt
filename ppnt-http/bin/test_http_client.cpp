
import std;
import ppnt.http.client;
import ppnt.http.http_types;
import ppnt.io.runtime;
import ppnt.io.task;
import ppnt.log;
import ppnt.traits;
import ppnt.err;
import ppnt.net.url;

using namespace ppnt;
using namespace ppnt::http;


auto async_main() -> io::Task<> {

    HttpClient client{};

    auto request = HttpRequest{};
    // auto url = net::Url::parse("https://tlspeet.ws/api/all");
    auto url = net::Url::parse("https://www.baidu.com");
    request.method = Method::GET;
    request.url = *url;
    auto res =  co_await client.request(std::move(request));
    if (!res) {
        log::error({"error: {}"}, res.error());
        co_return;
    }
    log::info({"response: {}"}, res->to_string());
    auto body = co_await res->body_full();
    if (!body) {
        log::error({"body error: {}"}, body.error());
    }
    std::string_view boby_view = {reinterpret_cast<const char *>(body->data()), body->size()};
    log::info({"boby_view: {}"}, boby_view);
    co_return ;
}

auto main() -> int {
    io::block_on(async_main());
    return 0;
}
