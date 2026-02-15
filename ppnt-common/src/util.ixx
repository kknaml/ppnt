export module ppnt.util;

import std;
import ppnt.traits;

export namespace ppnt {

    constexpr auto to_lower_ascii(char c) noexcept -> char {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
    }

    constexpr auto to_lower_ascii_str(std::string_view str) -> std::string {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            result += to_lower_ascii(c);
        }
        return result;
    }

    struct IgnoreCaseEqual {
        using is_transparent = void;
        static constexpr auto operator()(std::string_view a, std::string_view b) noexcept -> bool {
            return std::ranges::equal(a, b, [](char a, char b) {
               return to_lower_ascii(a) == to_lower_ascii(b);
            });
        }
    };

    struct IgnoreCaseHash {
        using is_transparent = void;

        constexpr auto operator()(std::string_view txt) const noexcept -> size_t {
            size_t h = 14695981039346656037ULL;
            for (char c : txt) {
                h ^= static_cast<size_t>(to_lower_ascii(c));
                h *= 1099511628211ULL;
            }
            return h;
        }
    };

    constexpr inline IgnoreCaseEqual ignore_case_equal{};
}

