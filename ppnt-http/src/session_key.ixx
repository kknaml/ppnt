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
        SessionKey(const SessionKey &other) : host(other.host), port(other.port), is_ssl(other.is_ssl),
        proxy(other.proxy){}

        auto operator=(SessionKey &&) noexcept -> SessionKey & = default;

        auto operator==(const SessionKey &) const -> bool = default;

        auto operator<=>(const SessionKey &) const = default;

    };
}