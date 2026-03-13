export module ppnt.http.client;

import std;
import ppnt.http.http1;
import ppnt.http.http2;
import ppnt.net.any_stream;
import ppnt.net.tcp_stream;
import ppnt.net.ssl;
import ppnt.net.tls;
import ppnt.net.addr;
import ppnt.io.task;
import ppnt.err;
import ppnt.traits;
import ppnt.http.http_types;
import ppnt.http.proxy;
import ppnt.log;
import ppnt.http.session;
import ppnt.http.session_key;

using namespace ppnt::net;

export namespace ppnt::http {

    struct HttpClientConfig {
        std::optional<ProxyConfig> proxy{};
        bool h2_enabled{true};
        net::ClientHelloSpecFactory *tls_spec_factory{nullptr};

        HttpTimeout timeout{};
    };


    class HttpClient {
    public:
        using Config = HttpClientConfig;

    private:
        Config config_;
        TlsContext tls_ctx_;
        std::shared_ptr<SessionPool> session_pool_;
        
    public:
        explicit HttpClient(Config config = {});

        // auto request(HttpRequest req) -> io::Task<Result<ClientResponse>>;
        auto request(HttpRequest req, std::optional<ProxyConfig> proxy_config = std::nullopt) -> io::TaskResult<HttpResponse<AnySession>>;
        auto close_async() -> io::Task<Result<Unit>>;
        
    private:
        //auto connect_to_remote(const std::string &host, int port, bool is_tls) -> io::Task<Result<BoxedStream>>;
    };
}
