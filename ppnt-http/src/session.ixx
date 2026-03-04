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
    };

    class SessionPool {

    public:
        auto acquire_session(const SessionKey &key) -> io::TaskResult<std::shared_ptr<AnySession>>;

        auto release_h1_session(const SessionKey &key, std::shared_ptr<AnySession> session) -> void;

    private:
        struct HostGroup {
            std::shared_ptr<AnySession> h2_session;
            std::deque<std::shared_ptr<AnySession>> h1_idle;
        };
        std::map<SessionKey, HostGroup> sessions_;
    };


    template <typename Impl>
    auto AnySession::create(Impl impl) -> std::unique_ptr<AnySession> {
        return std::make_unique<SessionWrapper<Impl>>(std::move(impl));
    }
}