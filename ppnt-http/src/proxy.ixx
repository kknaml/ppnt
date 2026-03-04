export module ppnt.http.proxy;

import std;
import ppnt.http.http_types;
import ppnt.common;

export namespace ppnt::http {

    struct ProxyAuth {
        std::string username;
        std::string password;

        ~ProxyAuth() {}

        auto operator=(const ProxyAuth &) -> ProxyAuth & = default;

        auto operator==(const ProxyAuth &) const -> bool = default;

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

        auto operator==(const ProxyConfig &) const -> bool = default;
    };
}