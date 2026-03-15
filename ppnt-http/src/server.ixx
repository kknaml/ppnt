export module ppnt.http.server;

import std;
import ppnt.common;
import ppnt.http.http_types;
import llhttp;
import ppnt.io.task;
import ppnt.http.session_key;

export namespace ppnt::http {

    template<Connection C>
    class Http1ServerSession : public NonCopy {
    private:
        C connection_;
        llhttp::llhttp_t parser_{};
        llhttp::llhttp_settings_t settings_{};

        HttpRequest request_{};
        std::vector<uint8_t> body_buffer_{};

        std::string temp_header_name_{};
        std::string temp_header_value_{};
        bool last_was_value_{false};
        bool headers_complete_{false};
        bool message_complete_{false};

    public:
        explicit Http1ServerSession(C connection) : connection_{std::move(connection)} {
            this->init();
        }

        auto receive_request(uint32_t timeout_ms = 0) -> io::TaskResult<HttpRequest> {
            reset_state();
            std::array<uint8_t, 4096> buf{};

            while (!headers_complete_) {
                auto read_res = co_await connection_.read(buf, timeout_ms);
                if (!read_res) co_return std::unexpected{read_res.error()};

                auto n = *read_res;
                if (n == 0) {
                    co_return make_err_result(std::errc::connection_aborted, "eof before headers complete");
                }

                auto err = llhttp::llhttp_execute(&parser_, reinterpret_cast<const char *>(buf.data()), n);
                if (err != llhttp::llhttp_errno::HPE_OK) {
                    co_return make_err_result(std::errc::bad_message, llhttp::llhttp_errno_name(err));
                }
            }

            request_.method = std::string(llhttp::llhttp_method_name(
                static_cast<llhttp::llhttp_method>(parser_.method)
            ));

            co_return std::move(this->request_);
        }

        auto receive_body() -> io::Task<Result<std::vector<uint8_t>>> {
            std::array<uint8_t, 4096> buf{};
            while (!message_complete_) {
                auto read_res = co_await connection_.read(buf);
                if (!read_res) co_return std::unexpected{read_res.error()};

                auto n = *read_res;
                if (n == 0) break;

                auto err = llhttp::llhttp_execute(&parser_, reinterpret_cast<const char *>(buf.data()), n);
                if (err != llhttp::llhttp_errno::HPE_OK) {
                    co_return make_err_result(std::errc::bad_message, "http1 request body");
                }
            }
            co_return std::move(this->body_buffer_);
        }

        auto send_response(auto response) -> io::Task<Result<Unit>> {
            auto payload = response.serialize_to_h1();
            auto res = co_await connection_.write(payload);
            if (!res) co_return std::unexpected{res.error()};


            if (!llhttp::llhttp_should_keep_alive(&parser_)) {
                this->close();
            }

            co_return Unit{};
        }

        auto close() -> void {
            connection_.close();
        }

    private:
        auto init() -> void {
            llhttp::llhttp_settings_init(&settings_);
            llhttp::llhttp_init(&parser_, llhttp::llhttp_type::HTTP_REQUEST, &settings_);
            set_callback_fn();
            parser_.data = this;
        }

        auto set_callback_fn() -> void {
            // Server 端特有：解析 URL
            settings_.on_url = on_url;

            settings_.on_header_field = on_header_field;
            settings_.on_header_value = on_header_value;
            settings_.on_headers_complete = on_headers_complete;
            settings_.on_body = on_body;
            settings_.on_message_complete = on_message_complete;
        }

        auto reset_state() -> void {
            llhttp::llhttp_reset(&parser_);
            request_ = HttpRequest{};
            body_buffer_.clear();
            temp_header_name_.clear();
            temp_header_value_.clear();
            last_was_value_ = false;
            headers_complete_ = false;
            message_complete_ = false;
        }

        auto commit_header() -> void {
            if (!temp_header_name_.empty()) {
                request_.headers.add(temp_header_name_, temp_header_value_);
                temp_header_name_.clear();
                temp_header_value_.clear();
            }
        }

        static auto on_url(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1ServerSession *>(p->data);
            self->request_.url.set_path(std::string_view(at, length));
            return 0;
        }

        static auto on_header_field(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1ServerSession *>(p->data);
            if (self->last_was_value_) {
                self->commit_header();
                self->last_was_value_ = false;
            }
            self->temp_header_name_.append(at, length);
            return 0;
        }

        static auto on_header_value(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1ServerSession *>(p->data);
            self->last_was_value_ = true;
            self->temp_header_value_.append(at, length);
            return 0;
        }

        static auto on_headers_complete(llhttp::llhttp_t *p) -> int {
            auto *self = static_cast<Http1ServerSession *>(p->data);
            if (self->last_was_value_) self->commit_header();
            self->headers_complete_ = true;
            return 0;
        }

        static auto on_body(llhttp::llhttp_t *p, const char *at, size_t length) -> int {
            auto *self = static_cast<Http1ServerSession *>(p->data);
            self->body_buffer_.insert(self->body_buffer_.end(), at, at + length);
            return 0;
        }

        static auto on_message_complete(llhttp::llhttp_t *p) -> int {
            auto *self = static_cast<Http1ServerSession *>(p->data);
            self->message_complete_ = true;
            return 0;
        }
    };
}