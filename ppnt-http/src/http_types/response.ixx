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
        std::shared_ptr<Session> session_owner_{};
        std::function<void(std::shared_ptr<Session>, bool)> body_consumed_hook_{};
        bool body_consumed_{false};
        HeadParts head_{};
        int id_{-1};
    public:
        explicit HttpResponse(Session *session) : session_{session} {}
        HttpResponse() : session_{nullptr} {}

        HttpResponse(HttpResponse &&other) noexcept
            : session_(std::exchange(other.session_, nullptr)),
              session_owner_(std::move(other.session_owner_)),
              body_consumed_hook_(std::move(other.body_consumed_hook_)),
              body_consumed_(std::exchange(other.body_consumed_, false)),
              head_(std::move(other.head_)),
              id_(std::exchange(other.id_, -1)) {}

        auto operator=(HttpResponse &&other) noexcept -> HttpResponse & {
            if (this != &other) {
                session_ = std::exchange(other.session_, nullptr);
                session_owner_ = std::move(other.session_owner_);
                body_consumed_hook_ = std::move(other.body_consumed_hook_);
                body_consumed_ = std::exchange(other.body_consumed_, false);
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

        auto set_session(std::shared_ptr<Session> session) noexcept -> void {
            session_ = session.get();
            session_owner_ = std::move(session);
        }

        auto set_body_consumed_hook(std::function<void(std::shared_ptr<Session>, bool)> hook) -> void {
            body_consumed_hook_ = std::move(hook);
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

        [[nodiscard]]
        auto get_id() const noexcept -> int {
            return id_;
        }

        auto set_id(int id) noexcept -> void {
            id_ = id;
        }

        auto body_full() -> io::Task<Result<std::vector<uint8_t>>> {
            auto result = co_await session_->body_full(*this);

            if (!body_consumed_) {
                body_consumed_ = true;
                if (body_consumed_hook_) {
                    body_consumed_hook_(session_owner_, result.has_value());
                }
            }

            co_return result;
        }

        auto to_string() const -> std::string {
            auto result = std::string{};
            result += std::format("{}", head_.status);
            result += "\n";
            for (const auto &header : this->head_.headers) {
                result += std::format("{}: {}\n", header.name, header.value);
            }
            return result;
        }
    };
}
