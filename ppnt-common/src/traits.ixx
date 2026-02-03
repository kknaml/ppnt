
export module ppnt.traits;

import std;

export namespace ppnt {

    using std::int8_t;
    using std::int16_t;
    using std::int32_t;
    using std::int64_t;
    using std::size_t;
    using std::uint8_t;
    using std::uint16_t;
    using std::uint32_t;
    using std::uint64_t;

    using Unit = std::monostate;

    template<typename T>
    using Regularized = std::conditional_t<std::is_void_v<T>, Unit, T>;

    class NonCopy {
    public:
        constexpr NonCopy() = default;
        NonCopy(const NonCopy &) = delete;
        NonCopy &operator=(const NonCopy &) = delete;
    };

    class NonInstance {
    public:
        NonInstance() = delete;
    };

    template<typename  T>
    class Singleton : public NonCopy {
    public:
        static T &instance() {
            static T inst;
            return inst;
        }
    protected:
        Singleton() = default;
        ~Singleton() = default;
    };

    template<typename T, typename R, typename ...Args>
    concept Func = std::is_invocable_r_v<R, T, Args...>;

    template<typename T, typename ...Args>
    concept Action = Func<T, void, Args...>;

    template<std::size_t N>
    struct FixedString {
        char buf[N + 1]{};

        explicit(false) constexpr FixedString(const char (&s)[N + 1]) {
            std::copy_n(s, N + 1, buf);
        }

        [[nodiscard]]
        constexpr const char *c_str() const {
            return buf;
        }
    };

    template<std::size_t N>
    FixedString(const char (&s)[N]) -> FixedString<N - 1>;

    template<typename T, typename P, typename R>
    concept HasAwaiterMembers = requires(T &t, std::coroutine_handle<std::conditional_t<std::is_same_v<P, Unit>, void, P>> h) {
        { t.await_ready() } -> std::convertible_to<bool>;
        t.await_suspend(h);
        { t.await_resume() } -> std::convertible_to<R>;
    };

    template<typename P, typename T>
    concept HasAwaitTransform = requires(P &p, T &&t) {
        p.await_transform(std::forward<T>(t));
    };

    template<typename T, typename R, typename Promise = Unit>
    concept Awaiterble =
        (std::same_as<Promise, Unit> && HasAwaiterMembers<T, Unit, R>)
        ||
        (!std::same_as<Promise, Unit> && (
          (HasAwaitTransform<Promise, T> &&
           requires(Promise& p, T &&t) {
               { p.await_transform(std::forward<T>(t)) } -> HasAwaiterMembers<Promise, R>;
           })
          ||
          (!HasAwaitTransform<Promise, T> && HasAwaiterMembers<T, Promise, R>)
        ));

    template<typename T>
    concept HasToString = requires(T &t) {
        { t.to_string() } -> std::convertible_to<std::string>;
    };

}

export namespace std {
    template<ppnt::HasToString T>
    struct formatter<T> {
        // Deduce the actual return type of to_string() (e.g., string, string_view, int, etc.)
        using UnderlyingType = std::remove_cvref_t<decltype(std::declval<T>().to_string())>;
        std::formatter<UnderlyingType> underlying;

        constexpr auto parse(format_parse_context& ctx) {
            return underlying.parse(ctx);
        }

        auto format(const T &t, format_context &ctx) const {
            return underlying.format(t.to_string(), ctx);
        }
    };
}

