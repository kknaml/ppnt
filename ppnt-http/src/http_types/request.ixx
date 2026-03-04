export module ppnt.http.http_types:request;

import std;
import ppnt.common;
import ppnt.io;
import :header;
import :method;


export namespace ppnt::http {


    struct HttpRequest : public NonCopy {
        std::string protocol = "http";
        std::string method = Method::GET;
        std::string path = "/";
        HttpHeaderList headers{};
        std::optional<std::vector<uint8_t>> body = std::nullopt;

        // gcc internal error when using = default;
        HttpRequest() {}

        HttpRequest(HttpRequest &&other) noexcept
        : method(std::move(other.method)), path(std::move(other.path)),
        headers(std::move(other.headers)), body(std::move(other.body)){}

        auto operator=(HttpRequest &&other) noexcept -> HttpRequest & {
            if (this != &other) {
                method = std::move(other.method);
                path = std::move(other.path);
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
            writer.write_string(std::format("{} {} HTTP/1.1\r\n", method, path));
            auto has_content_length = false;
            auto has_host = false;

            for (const auto &[name, value] : headers) {
                writer.write_string(std::format("{}: {}\r\n", name, value));
                if (!has_content_length && ignore_case_equal(name, "content-length")) has_content_length = true;
                if (!has_host && ignore_case_equal(name, "host")) has_host = true;
            }
            // std::ranges::for_each(headers, [&](const auto &header) {
            //     const auto &[name, value] = header;
            //     writer.write_string(std::format("{}: {}\r\n", name, value));
            //     if (!has_content_length && ignore_case_equal(name, "content-length")) {
            //         has_content_length = true;
            //     }
            //     if (!has_host && ignore_case_equal(name, "host")) {
            //         has_host = true;
            //     }
            // });
            if (body && !body->empty() && !has_content_length) {
                writer.write_string(std::format("content-length: {}\r\n", body->size()));
            }

            // \r\n
            writer.write_u16(0x0D0A, std::endian::big);

            if (body && !body->empty()) {
                writer.write_bytes(body->data(), body->size());
            }

            return std::move(writer).leak();
        }
    };
}