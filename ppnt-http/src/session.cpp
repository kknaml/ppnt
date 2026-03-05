module ppnt.http.session;

import ppnt.net.any_stream;
import ppnt.http.http1;
import ppnt.http.http2;
import ppnt.net.tcp_stream;
import ppnt.net.addr;
import ppnt.net.ssl;
import ppnt.net.tls;

namespace ppnt::http {

    namespace {
        auto is_h1(Version version) -> bool {
            return (version == Version::HTTP_11) || (version == Version::HTTP_10);
        }

        auto make_proxy_stream(const ProxyConfig &proxy_config, const SessionKey &key) -> io::TaskResult<net::TcpStream> {
            auto addr = net::resolve_first(proxy_config.host, proxy_config.port);
            if (!addr) {
                co_return std::unexpected{addr.error()};
            }
            auto tmp = co_await net::TcpStream::connect(*addr);
            if (!tmp) {
                co_return std::unexpected{tmp.error()};
            }
            auto tcp_stream = std::move(*tmp);

            auto connect_req_builder = Http1RequestBuilder{};
            connect_req_builder.method(Method::CONNECT);
            if (proxy_config.auth) {
                connect_req_builder.header(proxy_config.auth->basic_header());
            }
            for (const auto &header : proxy_config.headers) {
                connect_req_builder.header(header);
            }
            auto req = std::move(connect_req_builder).build();

            auto session = Http1Session<net::TcpStream>(std::move(tcp_stream), key);
            auto res = co_await session.request(std::move(req));

            if (!res) {
                co_return std::unexpected{res.error()};
            }
            if (res->get_status().code == 200) {
                co_return std::move(session).leak_stream();
            }
            co_return make_err_result(std::errc::not_connected, std::format("Proxy connect: {}", res->get_status().code));
        }

        auto session_connect(const SessionKey &key) -> io::TaskResult<std::shared_ptr<AnySession>> {
            net::TcpStream tcp_stream{};

            if (key.proxy) {
                auto tmp = co_await make_proxy_stream(*key.proxy, key);
                if (!tmp) {
                    co_return std::unexpected{tmp.error()};
                }
                tcp_stream = std::move(*tmp);
            } else {
                auto addr = net::resolve_first(key.host, key.port);
                if (!addr) {
                    co_return std::unexpected{addr.error()};
                }
                auto tmp = co_await net::TcpStream::connect(*addr);
                if (!tmp) {
                    co_return std::unexpected{tmp.error()};
                }
                tcp_stream = std::move(*tmp);
            }
            std::optional<net::BoxedStream> boxed_stream{std::nullopt};
            bool use_h2{false};
            if (key.is_ssl) {
                // TODO context
                auto ctx = net::TlsContext::client();
                if (!ctx) {
                    co_return std::unexpected{ctx.error()};
                }
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
                auto session = Http2Session(std::move(*boxed_stream), key);
                co_return AnySession::create(std::move(session));
            } else {
                auto session = Http1Session(std::move(*boxed_stream), key);
                co_return AnySession::create(std::move(session));
            }
        }
    }

    auto SessionPool::acquire_session(const SessionKey &key) -> io::TaskResult<std::shared_ptr<AnySession>> {
        auto &group = sessions_[key];
        if (group.h2_session) {
            if (group.h2_session->is_valid()) {
                co_return group.h2_session;
            } else {
                group.h2_session.reset();
            }
        }
        while (!group.h1_idle.empty()) {
            auto session = group.h1_idle.front();
            group.h1_idle.pop_front();

            if (session->is_valid()) {
                co_return session;
            }
        }

        auto new_session_ptr = co_await session_connect(key);
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
            const auto& key = session->get_session_key();
            if (sessions_[key].h1_idle.size() < 100) {
                sessions_[key].h1_idle.push_back(std::move(session));
            } else {
                session->close();
            }
        }
    }
}
