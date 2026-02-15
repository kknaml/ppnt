export module ppnt.http.proxy;

import std;

export namespace ppnt::http {

    struct ProxyAuth {
        std::string username;
        std::string password;

        ~ProxyAuth() {}

        auto operator=(const ProxyAuth &) -> ProxyAuth & = default;
    };

    struct ProxyConfig {
        std::string host;
        int port;
        std::optional<ProxyAuth> auth;

        auto operator<=>(const ProxyConfig &) const = default;
    };
}