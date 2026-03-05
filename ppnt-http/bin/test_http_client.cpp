
import std;
import ppnt.http.client;
import ppnt.http.http_types;
import ppnt.io.runtime;
import ppnt.io.task;
import ppnt.log;
import ppnt.traits;
import ppnt.err;
import ppnt.net.url;
import ppnt.http.proxy;

using namespace ppnt;
using namespace ppnt::http;

auto build_test_reqeust() -> HttpRequest {
    auto request = HttpRequest{};
    auto url = net::Url::parse("https://tls.peet.ws/api/all");
    // auto url = net::Url::parse("https://baidu.com");
    request.method = Method::GET;
    request.url = *url;

    log::debug({"url: {}"}, request.url);
    return request;
}

auto get_proxy() -> ProxyConfig {
    auto proxy = ProxyConfig{};
    proxy.host = "10.5.2.85";
    proxy.port = 9001;
    proxy.headers.add("Proxy-Authorization", "123456");
    return proxy;
}

auto async_main() -> io::Task<> {

    HttpClient client{};

    auto request = build_test_reqeust();
    auto res =  co_await client.request(std::move(request), get_proxy());
    if (!res) {
        log::error({"error: {}"}, res.error());
        co_return;
    }
    log::info({"response: \n{}"}, res->to_string());
    auto body = co_await res->body_full();
    if (!body) {
        log::error({"body error: {}"}, body.error());
    }
    std::string_view boby_view = {reinterpret_cast<const char *>(body->data()), body->size()};
    log::info({"boby_view: \n{}"}, boby_view);
    co_return ;
}

auto main() -> int {
    io::block_on(async_main());
    return 0;
}
