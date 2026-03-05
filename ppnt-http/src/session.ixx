export module ppnt.http.session;

import std;
import ppnt.common;
import ppnt.io.task;
import ppnt.http.http_types;
import ppnt.http.proxy;
import ppnt.http.session_key;

export namespace ppnt::http {

    class AnySession : public NonCopy {
    public:
        virtual ~AnySession() = default;

        [[nodiscard]]
        virtual auto get_session_key() const -> const SessionKey & = 0;

        [[nodiscard]]
        virtual auto get_http_version() const -> Version = 0;

        virtual auto is_valid() const -> bool = 0;

        virtual auto close() -> void = 0;

        virtual auto request(HttpRequest request) -> io::TaskResult<HttpResponse<AnySession>> = 0;

        virtual auto body_full(HttpResponse<AnySession> &resp) -> io::Task<Result<std::vector<uint8_t>>> = 0;

        template <typename Impl>
        static auto create(Impl impl) -> std::unique_ptr<AnySession>;
    };

    template<typename Impl>
    class SessionWrapper : public AnySession {
    private:
        Impl impl_;
    public:
        explicit SessionWrapper(Impl impl) : impl_(std::move(impl)) {}

        [[nodiscard]]
        auto get_session_key() const -> const SessionKey & override {
            return impl_.get_session_key();
        }

        [[nodiscard]]
        auto get_http_version() const -> Version override {
            return impl_.get_http_version();
        }

        auto is_valid() const -> bool override {
            return impl_.is_valid();
        }

        auto close() -> void override {
            impl_.close();
        }

        auto request(HttpRequest request) -> io::TaskResult<HttpResponse<AnySession>> override {// 1. 调用具体的实现 (比如 Http1Session::request)

            auto result = co_await impl_.request(std::move(request));

            if (!result) {
                co_return std::unexpected(result.error());
            }
            auto &inner_resp = *result;

            HttpResponse<AnySession> outer_resp(this);


            outer_resp.set_status(inner_resp.get_status());
            outer_resp.get_headers() = std::move(inner_resp).get_headers();


            co_return outer_resp;
        }

        auto body_full(HttpResponse<AnySession> &resp) -> io::Task<Result<std::vector<uint8_t>>> override {
            return impl_.body_full(resp);
        }
    };

    class SessionPool : NonCopy {

    public:
        SessionPool() = default;
        SessionPool(SessionPool &&others) noexcept : sessions_{std::move(others.sessions_)} {}
        auto operator=(SessionPool &&others) noexcept -> SessionPool & {
            if (this != &others) {
                sessions_ = std::move(others.sessions_);
            }
            return *this;
        }


        auto acquire_session(const SessionKey &key) -> io::TaskResult<std::shared_ptr<AnySession>>;

        auto release_h1_session(const SessionKey &key, std::shared_ptr<AnySession> session) -> void;

    private:
        struct HostGroup {
            std::shared_ptr<AnySession> h2_session{};
            std::deque<std::shared_ptr<AnySession>> h1_idle{};
        };
        std::map<SessionKey, HostGroup> sessions_{};
    };


    template <typename Impl>
    auto AnySession::create(Impl impl) -> std::unique_ptr<AnySession> {
        return std::make_unique<SessionWrapper<Impl>>(std::move(impl));
    }
}