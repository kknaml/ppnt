export module ppnt.log;

import std;
import ppnt.libc;

namespace detail {

    // template<typename... Args>
    // auto log_output(std::format_string<Args...> fmt, Args &&...args) -> void {
    //     std::println(fmt, std::forward<Args>(args)...);
    // }
}

export namespace ppnt::log {

    enum class Level : int {
        Debug,
        Info,
        Warn,
        Error
    };

    namespace colors {
        constexpr std::string_view RESET       = "\033[0m";
        constexpr std::string_view BOLD        = "\033[1m";
        constexpr std::string_view DIM         = "\033[2m";
        
        constexpr std::string_view RED         = "\033[31m";
        constexpr std::string_view GREEN       = "\033[32m";
        constexpr std::string_view YELLOW      = "\033[33m";
        constexpr std::string_view BLUE        = "\033[34m";
        constexpr std::string_view MAGENTA     = "\033[35m";
        constexpr std::string_view CYAN        = "\033[36m";
        constexpr std::string_view WHITE       = "\033[37m";
        
        constexpr std::string_view BRIGHT_RED     = "\033[91m";
        constexpr std::string_view BRIGHT_GREEN   = "\033[92m";
        constexpr std::string_view BRIGHT_YELLOW  = "\033[93m";
        constexpr std::string_view BRIGHT_BLUE    = "\033[94m";
        constexpr std::string_view BRIGHT_MAGENTA = "\033[95m";
        constexpr std::string_view BRIGHT_CYAN    = "\033[96m";
        constexpr std::string_view GRAY           = "\033[90m";
    }

    constexpr std::string_view get_color(Level level) {
        switch (level) {
            case Level::Info:  return colors::BRIGHT_GREEN;
            case Level::Warn:  return colors::BRIGHT_YELLOW;
            case Level::Error: return colors::BRIGHT_RED;
            case Level::Debug: return colors::BRIGHT_BLUE;
            default: return colors::RESET;
        }
    }

    constexpr std::string_view get_level_str(Level level) {
        switch (level) {
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERR ";
            case Level::Debug: return "DBUG";
            default: return "UNKN";
        }
    }

    template<typename... Args>
    struct FmtLoc {
        std::format_string<Args...> fmt;
        std::source_location loc = std::source_location::current();
    };

    template<Level level, typename... Args>
    void log_impl(FmtLoc<Args...> wrapper, Args &&...args) {
        // Use system_clock directly, let std::format handle the string conversion efficiently
        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        
        std::string_view full_path = wrapper.loc.file_name();
        auto filename = full_path.substr(full_path.find_last_of("/\\") + 1);

        if constexpr (level == Level::Error) {
            std::println(
                stderr,
                "{}[{:%H:%M:%S}] {}{}{} {}{}[{}:{}] {}{}",
                colors::GRAY, now,
                get_color(level), colors::BOLD, get_level_str(level),
                colors::RESET, colors::DIM, filename, wrapper.loc.line(), colors::RESET,
                std::format(wrapper.fmt, std::forward<Args>(args)...)
            );
        } else {
            std::println(
                "{}[{:%H:%M:%S}] {}{}{} {}{}[{}:{}] {}{}",
                colors::GRAY, now,
                get_color(level), colors::BOLD, get_level_str(level),
                colors::RESET, colors::DIM, filename, wrapper.loc.line(), colors::RESET,
                std::format(wrapper.fmt, std::forward<Args>(args)...)
            );
        }
    }

    auto set_level(Level level) -> void;
    auto get_level() -> Level;

    template<typename... Args>
    void info(FmtLoc<Args...> fmt, Args &&...args) {
        if (get_level() <= Level::Info) {
            log_impl<Level::Info>(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void warn(FmtLoc<Args...> fmt, Args &&...args) {
        if (get_level() <= Level::Warn) {
            log_impl<Level::Warn>(fmt, std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void error(FmtLoc<Args...> fmt, Args &&...args) {
        if (get_level() <= Level::Error) {
            log_impl<Level::Error>(fmt, std::forward<Args>(args)...);
        }
    }
    
    template<typename... Args>
    void debug(FmtLoc<Args...> fmt, Args &&...args) {
        if (get_level() <= Level::Debug) {
            log_impl<Level::Debug>(fmt, std::forward<Args>(args)...);
        }
    }
}
