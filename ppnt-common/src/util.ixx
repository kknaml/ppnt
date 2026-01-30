export module ppnt.util;

import std;
import ppnt.traits;

export namespace ppnt {

    struct IgnoreCaseEqual {

        static auto operator()(std::string_view a, std::string_view b) noexcept -> bool {
            return std::ranges::equal(a, b, [](char a, char b) {
               return std::tolower(static_cast<unsigned char>(a)) == static_cast<unsigned char>(b);
            });
        }
    };
}

