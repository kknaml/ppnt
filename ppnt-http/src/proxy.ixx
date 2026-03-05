export module ppnt.http.proxy;

import std;
import ppnt.http.http_types;
import ppnt.common;

export namespace ppnt::http {

    struct ProxyAuth {
        std::string username;
        std::string password;

        ProxyAuth(std::string username, std::string password) : username(std::move(username)), password(std::move(password)) {}

        ProxyAuth(const ProxyAuth &) = default;
        ProxyAuth(ProxyAuth &&) = default;

        auto operator=(ProxyAuth &&) -> ProxyAuth & = default;

        ~ProxyAuth() {}

        auto operator=(const ProxyAuth &) -> ProxyAuth & = default;

        auto operator==(const ProxyAuth &) const -> bool = default;

        auto operator<=>(const ProxyAuth &) const = default;

        [[nodiscard]]
        auto basic_auth_value() const -> std::string {
            return std::format("Basic {}",
                Base64::encode(std::format("{}:{}", username, password))
            );
        }

        [[nodiscard]]
        auto basic_header() const -> HttpHeader {
            return HttpHeader("Authorization", basic_auth_value());
        }
    };

    struct ProxyConfig {
        std::string host;
        int port;
        std::optional<ProxyAuth> auth;
        HttpHeaderList headers;


        auto operator<=>(const ProxyConfig& other) const -> std::strong_ordering {
            if (auto cmp = host <=> other.host; cmp != 0) {
                return cmp;
            }
            if (auto cmp = port <=> other.port; cmp != 0) {
                return cmp;
            }
            return auth <=> other.auth;
        }

        auto operator==(const ProxyConfig& other) const -> bool {
            return host == other.host &&
                   port == other.port &&
                   auth == other.auth;
        }
    };
}