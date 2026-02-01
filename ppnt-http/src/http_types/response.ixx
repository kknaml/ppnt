export module ppnt.http.http_types:response;

import std;
import ppnt.traits;
import ppnt.io.task;
import ppnt.err;
import :header;

export namespace ppnt::http {

    struct HttpStatus {
        int code{};
        std::string message{};

        auto to_string() const -> std::string {
            return std::format("{} {}", this->code, this->message);
        }
    };

    template<typename Session>
    class HttpResponse : public NonCopy {
    public:
       friend Session;
    private:
        Session *session_;
        HttpStatus http_status_{};
        HttpHeaderList headers_{};
        int id_{-1};
    public:
        explicit HttpResponse(Session *session) : session_{session} {}
        HttpResponse() : session_{nullptr} {}

        HttpResponse(HttpResponse &&other) noexcept : session_(std::exchange(other.session_, nullptr)),
            http_status_(std::move(other.get_status())), headers_(std::move(other.headers_)), id_(std::exchange(other.id_, -1)) {}

        auto operator=(HttpResponse &&other) -> HttpResponse & {
            if (this != &other) {
                session_ = std::exchange(other.session_, nullptr);
                http_status_ = other.http_status_;
                headers_ = std::move(other.headers_);
                id_ = std::exchange(other.id_, -1);
            }
            return *this;
        }

        auto get_session() const noexcept -> Session * {
            return session_;
        }

        auto set_session(Session *session) noexcept -> Session * {
            session_ = session;
        }

        auto get_headers(this auto &&self) -> decltype(auto) {
            return std::forward_like<decltype(self)>(self.headers_);
        }

        auto get_status() noexcept -> HttpStatus {
            return http_status_;
        }

        auto set_status(HttpStatus status) noexcept -> void {
            http_status_ = status;
        }

        auto body_full() -> io::Task<Result<std::vector<uint8_t>>> {
            return session_->body_full(*this);
        }

        auto to_string() const -> std::string {
            auto result = std::string{};
            result += std::format("{}", http_status_);
            result += "\n";
            for (const auto &header : this->headers_) {
                result += std::format("{}: {}\n", header.name, header.value);
            }
            result += "\n";
            return result;
        }
    };
}