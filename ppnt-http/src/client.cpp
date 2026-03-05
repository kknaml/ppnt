module ppnt.http.client;

using namespace ppnt::net;

namespace ppnt::http {

    HttpClient::HttpClient(Config config) : config_(std::move(config)), tls_ctx_(TlsContext::client().value()) {}

    auto HttpClient::request(HttpRequest req, std::optional<ProxyConfig> proxy_config) -> io::TaskResult<HttpResponse<AnySession>> {
        auto key = SessionKey(req.url.host(), req.url.port_or_default(), req.url.scheme() == "https", std::move(proxy_config));
        auto session = co_await session_pool_.acquire_session(key);
        if (!session) {
            co_return std::unexpected{session.error()};
        }
        co_return co_await (*session)->request(std::move(req));
    }

    // auto HttpClient::request(HttpRequest req) -> io::Task<Result<ClientResponse>> {
    //         // 1. Parse URL (Simplified: assume req has host/port hints or we parse them)
    //         auto host_iter = req.headers.get("host");
    //         if (!host_iter) co_return make_err_result(std::errc::destination_address_required, "Host header missing");
    //         std::string host(*host_iter);
    //         std::string hostname = host;
    //         int port = 80;
    //         bool is_tls = req.protocol == "https";
    //
    //         if (is_tls) port = 443;
    //
    //         auto colon = host.find(':');
    //         if (colon != std::string::npos) {
    //             hostname = host.substr(0, colon);
    //             port = std::stoi(host.substr(colon + 1));
    //         }
    //
    //         // 2. Connect
    //         auto stream_res = co_await connect_to_remote(hostname, port, is_tls);
    //         if (!stream_res) co_return std::unexpected{stream_res.error()};
    //
    //         // 3. Negotiate / Session
    //         // Create session on HEAP to keep it alive
    //         auto session = std::make_shared<Http1Session<BoxedStream>>(std::move(*stream_res));
    //
    //         // Proxy Plain HTTP rewriting
    //         if (config_.proxy && !is_tls) {
    //             req.path = std::format("http://{}:{}{}", hostname, port, req.path);
    //         }
    //
    //         auto res_result = co_await session->request(std::move(req));
    //         if (!res_result) co_return std::unexpected{res_result.error()};
    //
    //         // 4. Construct ClientResponse
    //         ClientResponse response;
    //         response.status = res_result->get_status();
    //         response.headers = std::move(res_result->get_headers());
    //
    //         // Erasure: Capture the session shared_ptr
    //         response.body_reader = [session = session, raw_res = std::move(*res_result)]() mutable -> io::Task<Result<std::vector<uint8_t>>> {
    //             auto res = co_await raw_res.body_full();
    //             if (res) {
    //                  session->close(); // Close on success? Or Keep-Alive logic here
    //             }
    //             co_return res;
    //         };
    //
    //         co_return response;
    // }
    
    auto HttpClient::connect_to_remote(const std::string &host, int port, bool is_tls) -> io::Task<Result<BoxedStream>> {
        // std::string target_host = config_.proxy ? config_.proxy->host : host;
        // int target_port = config_.proxy ? config_.proxy->port : port;
        //
        // // 1. TCP
        // auto tcp_res = co_await TcpStream::connect(target_host, target_port);
        // if (!tcp_res) co_return std::unexpected{tcp_res.error()};
        // TcpStream tcp = std::move(*tcp_res);
        //
        // // 2. Proxy Connect (Tunnel)
        // if (config_.proxy && is_tls) {
        //     // TODO: Implement CONNECT
        //         auto connect_req = std::format("CONNECT {}:{} HTTP/1.1\r\nHost: {}:{}\r\n\r\n", host, port, host, port);
        //         auto wres = co_await tcp.write(std::span{reinterpret_cast<const uint8_t*>(connect_req.data()), connect_req.size()});
        //         if (!wres) co_return std::unexpected{wres.error()};
        //
        //         // Read 200 OK (Simple skip)
        //         std::array<uint8_t, 1024> buf;
        //         // Ideally read line by line until \r\n\r\n
        //         // For now, just read once and hope it contains the full response.
        //         // Production grade needs a proper parser here.
        //         auto rres = co_await tcp.read(buf);
        //         if (!rres) co_return std::unexpected{rres.error()};
        //         // Check if 200...
        // }
        //
        // // 3. TLS
        // if (is_tls) {
        //     auto tls_res = co_await TlsStream::connect(std::move(tcp), tls_ctx_, host);
        //     if (!tls_res) co_return std::unexpected{tls_res.error()};
        //     co_return BoxedStream(std::move(*tls_res));
        // }
        //
        // // 4. Plain
        // co_return BoxedStream(std::move(tcp));
    }
}
