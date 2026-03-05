export module ppnt.http.http_types:request;

import std;
import ppnt.common;
import ppnt.io;
import ppnt.net.url;
import :header;
import :method;
import :connection;

export namespace ppnt::http {

    struct HttpRequest : public NonCopy {
        net::Url url;
        std::string method = Method::GET;
        HttpHeaderList headers{};
        std::optional<std::vector<uint8_t>> body = std::nullopt;
        HttpTimeout timeout;
        // gcc internal error when using = default;
        HttpRequest() {}

        HttpRequest(HttpRequest &&other) noexcept
        : method(std::move(other.method)), url(std::move(other.url)),
        headers(std::move(other.headers)), body(std::move(other.body)){}

        auto operator=(HttpRequest &&other) noexcept -> HttpRequest & {
            if (this != &other) {
                method = std::move(other.method);
                url = std::move(other.url);
                headers = std::move(other.headers);
                body = std::move(other.body);
            }
            return *this;
        }

        [[nodiscard]]
        auto serialize_to_h1() const -> std::vector<uint8_t> {
            auto writer = io::BinartWriter{};
            if (body) {
                writer.reserve(512 + body->size());
            }

            std::string request_target;
            bool is_connect = method == Method::CONNECT;

            if (is_connect) {
                auto port = url.port_or_default();
                if (port == 0) {
                    port = (url.scheme() == "https" || url.scheme() == "wss") ? 443 : 80;
                }
                request_target = std::format("{}:{}", url.host(), port);
            } else {
                request_target = url.path().empty() ? "/" : std::string{url.path()};
                if (!url.query().empty()) {
                    request_target += "?" + std::string{url.query()};
                }
            }

            writer.write_string(std::format("{} {} HTTP/1.1\r\n", method, request_target));

            auto has_content_length = false;
            auto has_host = false;

            for (const auto &[name, value]: headers) {
                writer.write_string(std::format("{}: {}\r\n", name, value));
                if (!has_content_length && ignore_case_equal(name, "content-length")) has_content_length = true;
                if (!has_host && ignore_case_equal(name, "host")) has_host = true;
            }

            if (!has_host) {
                if (is_connect) {
                    writer.write_string(std::format("Host: {}\r\n", request_target));
                } else {
                    auto port = url.port_or_default();
                    if (port != 0 && port != 80 && port != 443) {
                        writer.write_string(std::format("Host: {}:{}\r\n", url.host(), port));
                    } else {
                        writer.write_string(std::format("Host: {}\r\n", url.host()));
                    }
                }
            }

            if (body && !body->empty() && !has_content_length) {
                writer.write_string(std::format("Content-Length: {}\r\n", body->size()));
            }

            writer.write_u16(0x0D0A, std::endian::big);

            if (body && !body->empty()) {
                writer.write_bytes(body->data(), body->size());
            }

            return std::move(writer).leak();
        }
    };
}