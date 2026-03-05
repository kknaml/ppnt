export module ppnt.http.http1;

import std;
import ppnt.http.http_types;
import ppnt.traits;
import ppnt.err;
import llhttp;
import ppnt.io.task;
import ppnt.http.session_key;

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
            req_.url.set_path(p);
            return *this;
        }

        auto header(std::string_view name, std::string_view value) -> Http1RequestBuilder & {
            req_.headers.add(name, value);
            return *this;
        }

        auto header(HttpHeader header) -> Http1RequestBuilder & {
            req_.headers.add(std::move(header));
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
    class Http1Session : public NonCopy {
    public:
        friend class HttpResponse<Http1Session>;
    private:
        C connection_;
        llhttp::llhttp_t parser_{};
        llhttp::llhttp_settings_t settings_{};

        std::vector<uint8_t> body_buffer_{};
        HttpResponse<Http1Session> response_{};
        std::string temp_header_name_{};
        std::string temp_header_value_{};
        bool last_was_value_{false};
        bool headers_complete_{false};
        bool message_complete_{false};

        SessionKey session_key_;

    public:
        explicit Http1Session(C connection, SessionKey session_key)
        : connection_(std::move(connection)), session_key_(std::move(session_key)) {
            this->init();
        }

        Http1Session(Http1Session &&other) noexcept
            : connection_(std::move(other.connection_)), parser_(other.parser_), settings_(other.settings_),
            body_buffer_(std::move(other.body_buffer_)), response_(std::move(other.response_)),
            temp_header_name_(std::move(other.temp_header_name_)), temp_header_value_(std::move(other.temp_header_value_)),
            last_was_value_(other.last_was_value_), headers_complete_(other.headers_complete_),
            message_complete_(other.message_complete_), session_key_(std::move(other.session_key_)) {
            parser_.settings = &settings_;
            parser_.data = this;
            response_.set_session(this);
        }

        auto operator=(Http1Session &&other) -> Http1Session & {
            if (this != &other) {
                connection_ = std::move(other.connection_);
                parser_ = std::move(other.parser_);
                settings_ = other.settings_;
                body_buffer_ = std::move(other.body_buffer_);
                response_ = std::move(other.response_);
                temp_header_name_ = std::move(other.temp_header_name_);
                temp_header_value_ = std::move(other.temp_header_value_);
                last_was_value_ = other.last_was_value_;
                headers_complete_ = other.headers_complete_;
                message_complete_ = other.message_complete_;
                session_key_ = std::move(other.session_key_);

                parser_.settings = &settings_;
                parser_.data = this;
                response_.set_session(this);
            }
            return *this;
        }

        auto request(HttpRequest request) -> io::Task<Result<HttpResponse<Http1Session>>> {
            reset_state();
            auto payload = request.serialize_to_h1();

            auto res = co_await connection_.write(payload, request.timeout.write_timeout);
            if (!res) co_return std::unexpected{res.error()};
            std::array<uint8_t, 4096> buf{};
            while (!headers_complete_) {
                auto read_res = co_await connection_.read(buf, request.timeout.read_timeout);
                if (!read_res) co_return std::unexpected{read_res.error()};
                auto n = *read_res;
                if (n == 0) break; // EOF
                auto err = llhttp::llhttp_execute(&parser_, reinterpret_cast<const char *>(buf.data()), n);
                if (err != llhttp::llhttp_errno::HPE_OK) {
                    co_return make_err_result(std::errc::bad_message, "http1 request");
                }
            }

            co_return std::move(this->response_);
        }

        template<typename Session>
        auto body_full(HttpResponse<Session> &resp) -> io::Task<Result<std::vector<uint8_t>>> {
            std::array<uint8_t, 4096> buf{};
            while (!message_complete_) {
                auto read_res = co_await connection_.read(buf);
                if (!read_res) co_return std::unexpected{read_res.error()};

                auto n = *read_res;
                if (n == 0) break;
                auto err = llhttp::llhttp_execute(&parser_, reinterpret_cast<const char *>(buf.data()), n);
                if (err != llhttp::llhttp_errno::HPE_OK) {
                    co_return make_err_result(std::errc::bad_message, "http1 body");
                }
            }

            co_return std::move(this->body_buffer_);
        }

        [[nodiscard]]
        auto is_valid() const -> bool {
            // TODO
            return true;
        }

        [[nodiscard]]
        auto get_http_version() const -> Version {
            return Version::HTTP_11;
        }

        [[nodiscard]]
        auto get_session_key() const -> const SessionKey & {
            return session_key_;
        }

        auto close() -> void {
            connection_.close();
        }

        auto leak_stream() && -> C {
            return std::move(connection_);
        }

    private:

        auto init() -> void {
            llhttp::llhttp_settings_init(&settings_);
            llhttp::llhttp_init(&parser_, llhttp::llhttp_type::HTTP_RESPONSE, &settings_);
            set_callback_fn();
            parser_.data = this;
            response_ = HttpResponse(this);
        }

        auto set_callback_fn() -> void {
            settings_.on_message_begin = on_message_begin;
            settings_.on_status = on_status;
            settings_.on_header_field = on_header_field;
            settings_.on_header_value = on_header_value;
            settings_.on_headers_complete = on_headers_complete;
            settings_.on_body = on_body;
            settings_.on_message_complete = on_message_complete;
        }

        auto reset_state() -> void {
            llhttp::llhttp_reset(&parser_);
            response_ = HttpResponse{this};
            temp_header_name_.clear();
            temp_header_value_.clear();
            last_was_value_ = false;
            message_complete_ = false;
        }

        auto commit_header() -> void {
            if (!temp_header_name_.empty()) {
                auto &headers = response_.get_headers();
                headers.add(temp_header_name_, temp_header_value_);
                temp_header_name_.clear();
                temp_header_value_.clear();
            }
        }

        static auto on_message_begin(llhttp::llhttp_t *p) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            return 0;
        }

        static auto on_status(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            self->response_.set_status(HttpStatus{p->status_code, std::string{at, length}});
            return 0;
        }

        static auto on_header_field(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            if (self->last_was_value_) {
                self->commit_header();
                self->last_was_value_ = false;
            }
            self->temp_header_name_.append(at, length);
            return 0;
        }

        static auto on_header_value(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            self->last_was_value_ = true;
            self->temp_header_value_.append(at, length);
            return 0;
        }

        static auto on_headers_complete(llhttp::llhttp_t *p) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            if (self->last_was_value_) self->commit_header();
            self->headers_complete_ = true;
            return 0;
        }

        static auto on_body(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            self->body_buffer_.insert(self->body_buffer_.end(), at, at + length);
            return 0;
        }

        static auto on_message_complete(llhttp::llhttp_t *p) -> int {
            auto *self = static_cast<Http1Session *>(p->data);
            self->message_complete_ = true;
            return 0;
        }
    };

    template<Connection C>
    Http1Session(C) -> Http1Session<C>;
}
