export module ppnt.http.client;

import std;
import ppnt.http.http1;
import ppnt.http.http2;
import ppnt.net.any_stream;
import ppnt.net.tcp_stream;
import ppnt.net.ssl;
import ppnt.net.addr;
import ppnt.io.task;
import ppnt.err;
import ppnt.traits;
import ppnt.http.http_types;
import ppnt.http.proxy;
import ppnt.log;

using namespace ppnt::net;

export namespace ppnt::http {

    struct HttpClientConfig {
        std::optional<ProxyConfig> proxy{};
        bool h2_enabled{true};
        // Duration connect_timeout...
    };


    class HttpClient {
    public:
        using Config = HttpClientConfig;

    private:
        Config config_;
        TlsContext tls_ctx_;
        // Connection Pools
        // Key: host, port, is_tls
        // We need a complex key for pool
        // Simplified for this example: Just direct connection or basic reuse if I had a pool.
        // For production, you'd have a map<Key, vector<BoxedStream>>.
        
    public:
        explicit HttpClient(Config config = {});

        // auto request(HttpRequest req) -> io::Task<Result<ClientResponse>>;
        
    private:
        auto connect_to_remote(const std::string &host, int port, bool is_tls) -> io::Task<Result<BoxedStream>>;
    };
}
