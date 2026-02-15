
import std;
import ppnt.http.client;
import ppnt.http.http_types;
import ppnt.io.runtime;
import ppnt.io.task;
import ppnt.log;
import ppnt.traits;
import ppnt.err;

using namespace ppnt;
using namespace ppnt::http;


auto async_main() -> io::Task<> {
    HttpClient client;

    // Test 1: Plain HTTP
    log::info({"--- Testing HTTP ---"});
    HttpRequest req_http{};
    req_http.method = "GET";
    req_http.path = "/";
    req_http.headers.add("Host", "example.com");
    
    auto res_http = co_await client.request(std::move(req_http));
    if (res_http) {
        log::info({"HTTP Status: {}"}, res_http->status.code);
        auto body = co_await res_http->text(); 
        if (body) {
            log::info({"HTTP Body len: {}"}, body->size());
        }
    } else {
            log::error({"HTTP Failed: {}"}, res_http.error());
    }

    // Test 2: HTTPS
    log::info({"--- Testing HTTPS ---"});
    HttpRequest req_https{};
    req_https.protocol = "https";
    req_https.method = "GET";
    req_https.path = "/";
    req_https.headers.add("Host", "example.com");

    auto res_https = co_await client.request(std::move(req_https));
    if (res_https) {
        log::info({"HTTPS Status: {}"}, res_https->status.code);
        log::info({"HTTPS Headers: {}"}, res_https->headers);
            auto body = co_await res_https->text();
            if (body) {
            log::info({"HTTPS Body len: {}"}, body->size());
        }
    } else {
        log::error({"HTTPS Failed: {}"}, res_https.error());
    }
}

auto main() -> int {
    io::block_on(async_main());
    return 0;
}
