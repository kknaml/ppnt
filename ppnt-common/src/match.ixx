export module ppnt.match;

import std;
import ppnt.traits;

export namespace ppnt {

    using None = std::monostate;

    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    template<typename T>
    concept OptionProtocol = requires(T t) {
        { static_cast<bool>(t) };
        { *t };
    };

    template<typename T>
    concept SumTypeProtocol = requires {
        typename std::variant_size<std::remove_cvref_t<T>>::type;
    };

    struct StrategySum {};
    struct StrategyOption {};
    struct StrategyValue {};

    template<typename T>
    constexpr auto detect_strategy() {
        using RawT = std::remove_cvref_t<T>;
        if constexpr (SumTypeProtocol<RawT>) {
            return StrategySum{};
        } else if constexpr (OptionProtocol<RawT>) {
            return StrategyOption{};
        } else {
            return StrategyValue{};
        }
    }

    template<typename T, typename Visitor>
    auto match_impl(T &&target, Visitor &&visitor, StrategySum) {
        return std::visit(std::forward<Visitor>(visitor), std::forward<T>(target));
    }

    template<typename T, typename Visitor>
    auto match_impl(T &&target, Visitor &&visitor, StrategyOption) {
        if (target) {
            return std::forward<Visitor>(visitor)(*std::forward<T>(target));
        } else {
            return std::forward<Visitor>(visitor)(None{});
        }
    }

    template<typename T, typename Visitor>
    auto match_impl(T&& target, Visitor&& visitor, StrategyValue) {
        return std::forward<Visitor>(visitor)(std::forward<T>(target));
    }



    template<typename T, typename ...Args>
    auto match(T &&target, Args &&...lambdas) {
        auto visitor = overloaded{std::forward<Args>(lambdas)...};
        constexpr auto strategy = detect_strategy<T>();
        return match_impl(std::forward<T>(target), visitor, strategy);
    }
}
