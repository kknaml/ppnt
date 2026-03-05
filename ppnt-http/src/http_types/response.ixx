export module ppnt.http.http_types:response;

import std;
import ppnt.common;
import ppnt.io.task;
import :header;
import :version;

export namespace ppnt::http {

    struct HttpStatus {
        int code{};
        std::string message{};

        auto to_string() const -> std::string {
            return std::format("{} {}", this->code, this->message);
        }
    };

    struct HeadParts {
        HttpStatus status{};
        Version version{Version::HTTP_11};
        HttpHeaderList headers{};
    };

    template<typename Session>
    class HttpResponse : public NonCopy {
    public:
       friend Session;
    private:
        Session *session_;
        HeadParts head_{};
        int id_{-1};
    public:
        explicit HttpResponse(Session *session) : session_{session} {}
        HttpResponse() : session_{nullptr} {}

        HttpResponse(HttpResponse &&other) noexcept : session_(std::exchange(other.session_, nullptr)),
            head_(std::move(other.head_)), id_(std::exchange(other.id_, -1)) {}

        auto operator=(HttpResponse &&other) noexcept -> HttpResponse & {
            if (this != &other) {
                session_ = std::exchange(other.session_, nullptr);
                head_ = std::move(other.head_);
                id_ = std::exchange(other.id_, -1);
            }
            return *this;
        }
        auto get_session() const noexcept -> Session * {
            return session_;
        }

        auto set_session(Session *session) noexcept -> void {
            session_ = session;
        }

        auto get_headers(this auto &&self) -> decltype(auto) {
            return std::forward_like<decltype(self)>(self.head_.headers);
        }

        auto get_status() const noexcept -> const HttpStatus & {
            return head_.status;
        }

        auto set_status(HttpStatus status) noexcept -> void {
            head_.status = std::move(status);
        }

        auto get_id() const noexcept -> int {
            return id_;
        }

        auto body_full() -> io::Task<Result<std::vector<uint8_t>>> {
            return session_->body_full(*this);
        }

        auto to_string() const -> std::string {
            auto result = std::string{};
            result += std::format("{}", head_.status);
            result += "\n";
            for (const auto &header : this->head_.headers) {
                result += std::format("{}: {}\n", header.name, header.value);
            }
            result += "\n";
            return result;
        }
    };
}