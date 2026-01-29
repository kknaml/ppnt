module;

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

import std;

export module pplog;

export namespace pplog {

    struct FormatWithLoc {
        std::string_view fmt;
        std::source_location loc;


        FormatWithLoc(std::string_view fmt, const std::source_location &loc = std::source_location::current()) : fmt(fmt), loc(loc) {}
    };


    template <typename... Args>
     void info(FormatWithLoc wrapper, Args&&... args) {

        spdlog::source_loc sloc{
            wrapper.loc.file_name(),
            static_cast<int>(wrapper.loc.line()),
            wrapper.loc.function_name()
        };
        auto view = spdlog::string_view_t{wrapper.fmt};
        spdlog::default_logger_raw()->log(
            sloc,
            spdlog::level::info,
            fmt::runtime(view),
            std::forward<Args>(args)...
        );
    }
}