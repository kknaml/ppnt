module ppnt.http.session;

import ppnt.net.any_stream;
import ppnt.http.http1;
import ppnt.http.http2;
import ppnt.net.tcp_stream;
import ppnt.net.addr;
import ppnt.net.ssl;
import ppnt.net.tls;
import ppnt.net.url;

namespace ppnt::http {

    namespace {
        auto is_h1(Version version) -> bool {
            return (version == Version::HTTP_11) || (version == Version::HTTP_10);
        }

        auto make_proxy_stream(net::Url target, const ProxyConfig &proxy_config, const SessionKey &key) -> io::TaskResult<net::TcpStream> {
            auto addr = net::resolve_first(proxy_config.host, proxy_config.port);
            if (!addr) {
                co_return std::unexpected{addr.error()};
            }
            auto tmp = co_await net::TcpStream::connect(*addr);
            if (!tmp) {
                co_return std::unexpected{tmp.error()};
            }
            auto tcp_stream = std::move(*tmp);

            auto connect_req = HttpRequest{};
            connect_req.method = Method::CONNECT;
            if (proxy_config.auth) {
                connect_req.headers.add(proxy_config.auth->basic_header());
            }
            for (const auto &header : proxy_config.headers) {
                connect_req.headers.add(header);
            }
            connect_req.url = std::move(target);

            auto session = Http1Session<net::TcpStream>(std::move(tcp_stream), key);
            auto res = co_await session.request(std::move(connect_req));

            if (!res) {
                co_return std::unexpected{res.error()};
            }
            if (res->get_status().code == 200) {
                co_return std::move(session).leak_stream();
            }
            co_return make_err_result(std::errc::not_connected, std::format("Proxy connect: {}", res->get_status().code));
        }

        auto session_connect(const SessionKey &key, uint32_t timeout_ms, net::TlsContext *tls_ctx) -> io::TaskResult<std::shared_ptr<AnySession>> {
            net::TcpStream tcp_stream{};

            if (key.proxy) {
                auto target_url = net::Url::parse(std::format("http://{}:{}", key.host, key.port));
                if (!target_url) {
                    co_return std::unexpected{target_url.error()};
                }
                auto tmp = co_await make_proxy_stream(std::move(*target_url), *key.proxy, key);
                if (!tmp) {
                    co_return std::unexpected{tmp.error()};
                }
                tcp_stream = std::move(*tmp);
            } else {
                auto addr = net::resolve_first(key.host, key.port);
                if (!addr) {
                    co_return std::unexpected{addr.error()};
                }
                auto tmp = co_await net::TcpStream::connect(*addr, timeout_ms);
                if (!tmp) {
                    co_return std::unexpected{tmp.error()};
                }
                tcp_stream = std::move(*tmp);
            }
            std::optional<net::BoxedStream> boxed_stream{std::nullopt};
            bool use_h2{false};
            if (key.is_ssl) {
                net::TlsContext *ctx = tls_ctx;
                std::optional<net::TlsContext> owned_ctx{};
                if (ctx == nullptr) {
                    auto ctx_res = net::TlsContext::client();
                    if (!ctx_res) {
                        co_return std::unexpected{ctx_res.error()};
                    }
                    owned_ctx = std::move(*ctx_res);
                    ctx = std::addressof(*owned_ctx);
                }
                // TODO timeout
                auto tmp = co_await net::TlsStream<>::connect(std::move(tcp_stream), *ctx, key.host, true);
                if (!tmp) {
                    co_return std::unexpected{tmp.error()};
                }
                use_h2 = tmp->is_h2();
                boxed_stream = net::BoxedStream{std::move(*tmp)};
            } else {
                boxed_stream = net::BoxedStream{std::move(tcp_stream)};
            }
            if (use_h2) {
                co_return AnySession::create<Http2Session<net::BoxedStream>>(std::move(*boxed_stream), key);
            } else {
                co_return AnySession::create<Http1Session<net::BoxedStream>>(std::move(*boxed_stream), key);
            }
        }
    }

    auto SessionPool::acquire_session(const SessionKey &key, uint32_t timeout_ms, net::TlsContext *tls_ctx) -> io::TaskResult<std::shared_ptr<AnySession>> {
        auto &group = sessions_[key];
        if (group.h2_session && !group.h2_session->is_valid()) {
            group.h2_session.reset();
        }

        auto invalid_range = std::ranges::remove_if(
            group.h1_idle,
            [](const std::shared_ptr<AnySession> &session) {
                return !session || !session->is_valid();
            }
        );
        group.h1_idle.erase(invalid_range.begin(), invalid_range.end());

        if (group.h2_session) {
            co_return group.h2_session;
        }
        while (!group.h1_idle.empty()) {
            auto session = std::move(group.h1_idle.front());
            group.h1_idle.pop_front();

            if (session->is_valid()) {
                co_return session;
            }
        }

        auto new_session_ptr = co_await session_connect(key, timeout_ms, tls_ctx);
        if (!new_session_ptr) co_return std::unexpected{new_session_ptr.error()};

        std::shared_ptr<AnySession> shared_session = std::move(*new_session_ptr);


        if (shared_session->get_http_version() == Version::HTTP_2) {
            group.h2_session = shared_session;
        }

        co_return shared_session;
    }

    auto SessionPool::release_h1_session(const SessionKey &key, std::shared_ptr<AnySession> session) -> void {
        if (!session || !session->is_valid()) return;

        if (is_h1(session->get_http_version())) {
            if (sessions_[key].h1_idle.size() < 100) {
                sessions_[key].h1_idle.push_back(std::move(session));
            } else {
                session->close();
            }
        }
    }

    auto SessionPool::close_async() -> io::Task<Result<Unit>> {
        for (auto &[key, group] : sessions_) {
            if (group.h2_session) {
                auto res = co_await group.h2_session->close_async();
                if (!res) {
                    co_return std::unexpected{res.error()};
                }
                group.h2_session.reset();
            }

            while (!group.h1_idle.empty()) {
                auto session = std::move(group.h1_idle.front());
                group.h1_idle.pop_front();
                if (!session) {
                    continue;
                }
                auto res = co_await session->close_async();
                if (!res) {
                    co_return std::unexpected{res.error()};
                }
            }
        }
        sessions_.clear();
        co_return {};
    }
}
