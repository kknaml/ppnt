export module ppnt.http.http1;

import std;
import ppnt.http.http_types;
import ppnt.traits;
import ppnt.err;
import llhttp;
import ppnt.io.task;

export namespace ppnt::http {

    class Http1RequestBuilder {
    private:
        HttpRequest req_{};
    public:
        Http1RequestBuilder() = default;

        auto method(std::string_view m) -> Http1RequestBuilder & {
            req_.method = m;
            return *this;
        }

        auto path(std::string_view p) -> Http1RequestBuilder & {
            req_.path = p;
            return *this;
        }

        auto header(std::string_view name, std::string_view value) -> Http1RequestBuilder & {
            req_.headers.add(name, value);
            return *this;
        }

        auto body(std::vector<uint8_t> body) -> Http1RequestBuilder & {
            req_.body = std::move(body);
            return *this;
        }

        [[nodiscard]]
        auto build() && -> HttpRequest {
            return std::move(req_);
        }
    };

    template<Connection C>
    class Http1Session {
    private:
        C connection_;
        llhttp::llhttp_t parser_{};
        llhttp::llhttp_settings_t settings_{};

    public:
        explicit Http1Session(C connection) : connection_(std::move(connection)) {
            llhttp::llhttp_settings_init(&settings_);
            settings_.on_message_begin = on_message_begin;
            settings_.on_status = on_status;
            settings_.on_header_field = on_header_field;
            settings_.on_header_value = on_header_value;
            settings_.on_headers_complete = on_headers_complete;
            settings_.on_body = on_body;
            settings_.on_message_complete = on_message_complete;

            llhttp::llhttp_init(&parser_, llhttp::llhttp_type::HTTP_RESPONSE, &settings_);
            parser_.data = this;
        }

        auto request(HttpRequest request) -> io::Task<Result<Unit>> { // TODO
            auto payload = request.serialize_to_h1();

            auto res = co_await connection_.write(payload);
            if (!res) co_return std::unexpected{res.error()};
            std::array<uint8_t, 4096> buf{};

            co_return{};
        }

        auto close() -> void {
            connection_.close();
        }

    private:
        static auto on_message_begin(llhttp::llhttp_t *p) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            return 0;
        }

        static auto on_status(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            return 0;
        }

        static auto on_header_field(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            return 0;
        }

        static auto on_header_value(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            return 0;
        }

        static auto on_headers_complete(llhttp::llhttp_t *p) -> int {
            return 0;
        }

        static auto on_body(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            return 0;
        }

        static auto on_message_complete(llhttp::llhttp_t *p) -> int {
            return 0;
        }
    };

    template<Connection C>
    Http1Session(C) -> Http1Session<C>;
}
