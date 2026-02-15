export module ppnt.http.session;

import std;
import ppnt.common;

export namespace ppnt::http {

    class AnySession : public NonCopy {
    public:
        virtual ~AnySession() = default;

        template <typename Impl>
        static auto create(Impl impl) -> std::unique_ptr<AnySession>;
    };

    template<typename Impl>
    class SessionWrapper : public AnySession {
    private:
        Impl impl_;
    public:
        explicit SessionWrapper(Impl impl) : impl_(std::move(impl)) {}


    };

    template <typename Impl>
    auto AnySession::create(Impl impl) -> std::unique_ptr<AnySession> {
        return std::make_unique<SessionWrapper<Impl>>(std::move(impl));
    }
}