export module ppnt.net.url;

import std;
import ppnt.common;

export namespace ppnt::net {

    enum class UrlError {
        InvalidUrl,
        InvalidBaseUr,
        ParseFailed
    };

    class Url {
    public:
    private:
        std::string scheme_{};
        std::string user_info_{};
        std::string host_{};
        int port_ {-1};
        std::string path_{};
        std::string query_{};
        std::string fragment_{};
   public:
        Url() = default;

        static auto parse(std::string_view input) -> Result<Url>;
        static auto resolve(std::string_view relative, const Url &base) -> Result<Url>;

        [[nodiscard]]
        auto href() const -> std::string;
        [[nodiscard]]
        auto origin() const -> std::string;

        [[nodiscard]]
        auto scheme() const -> std::string {
            return scheme_;
        }
        [[nodiscard]]
        auto host() const -> std::string {
            return host_;
        }
        [[nodiscard]]
        auto path() const -> std::string {
            return path_;
        }
        [[nodiscard]]
        auto query() const -> std::string {
            return query_;
        }
        [[nodiscard]]
        auto fragment() const -> std::string {
            return fragment_;
        }
        [[nodiscard]]
        auto port_or_default() const -> int {
            if (port_ != -1) return port_;
            if (scheme_ == "http") return 80;
            if (scheme_ == "https") return 443;
            return 0;
        }

        [[nodiscard]]
        auto to_string() const -> std::string;

    private:
        static auto parse_authority(std::string_view authority, Url &url) -> Result<Unit>;
    };
}