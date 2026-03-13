export module ppnt.http.session;

import std;
import ppnt.common;
import ppnt.io.task;
import ppnt.http.http_types;
import ppnt.http.proxy;
import ppnt.http.session_key;
import ppnt.net.ssl;

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

        virtual auto close_async() -> io::Task<Result<Unit>> = 0;

        virtual auto request(HttpRequest request) -> io::TaskResult<HttpResponse<AnySession>> = 0;

        virtual auto body_full(HttpResponse<AnySession> &resp) -> io::Task<Result<std::vector<uint8_t>>> = 0;

        template <typename Impl, typename... Args>
        static auto create(Args &&...args) -> std::unique_ptr<AnySession>;
    };

    template<typename Impl>
    class SessionWrapper : public AnySession {
    private:
        std::unique_ptr<Impl> impl_;
    public:
        explicit SessionWrapper(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

        [[nodiscard]]
        auto get_session_key() const -> const SessionKey & override {
            return impl_->get_session_key();
        }

        [[nodiscard]]
        auto get_http_version() const -> Version override {
            return impl_->get_http_version();
        }

        auto is_valid() const -> bool override {
            return impl_->is_valid();
        }

        auto close() -> void override {
            impl_->close();
        }

        auto close_async() -> io::Task<Result<Unit>> override {
            co_return co_await impl_->close_async();
        }

        auto request(HttpRequest request) -> io::TaskResult<HttpResponse<AnySession>> override {

            auto result = co_await impl_->request(std::move(request));

            if (!result) {
                co_return std::unexpected(result.error());
            }
            auto &inner_resp = *result;

            HttpResponse<AnySession> outer_resp(this);


            outer_resp.set_status(inner_resp.get_status());
            outer_resp.set_id(inner_resp.get_id());
            outer_resp.get_headers() = std::move(inner_resp).get_headers();

            co_return outer_resp;
        }

        auto body_full(HttpResponse<AnySession> &resp) -> io::Task<Result<std::vector<uint8_t>>> override {
            return impl_->body_full(resp);
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


        auto acquire_session(const SessionKey &key, uint32_t timeout_ms = 0, net::TlsContext *tls_ctx = nullptr) -> io::TaskResult<std::shared_ptr<AnySession>>;

        auto release_h1_session(const SessionKey &key, std::shared_ptr<AnySession> session) -> void;

        auto close_async() -> io::Task<Result<Unit>>;

    private:
        struct HostGroup {
            std::shared_ptr<AnySession> h2_session{};
            std::deque<std::shared_ptr<AnySession>> h1_idle{};
        };
        std::map<SessionKey, HostGroup> sessions_{};
    };


    template <typename Impl, typename... Args>
    auto AnySession::create(Args &&...args) -> std::unique_ptr<AnySession> {
        return std::make_unique<SessionWrapper<Impl>>(
            std::make_unique<Impl>(std::forward<Args>(args)...)
        );
    }
}
