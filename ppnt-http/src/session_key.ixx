export module ppnt.http.session_key;

import std;
import ppnt.http.proxy;

export namespace ppnt::http {
    struct SessionKey {
        std::string host;
        int port;
        bool is_ssl;
        std::optional<ProxyConfig> proxy;

        SessionKey() = default;
        SessionKey(std::string host, int port, bool is_ssl, std::optional<ProxyConfig> proxy)
            : host(std::move(host)), port(port), is_ssl(is_ssl), proxy(std::move(proxy)) {}
        SessionKey(SessionKey &&) noexcept = default;
        auto operator=(SessionKey &&) noexcept = default;

        auto operator==(const SessionKey &) const -> bool = default;
    };
}