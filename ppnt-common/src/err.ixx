export module ppnt.err;

import std;
import ppnt.traits;

export namespace ppnt {

    struct Error {
        std::error_code ec;
        std::source_location loc;
        std::string message_;
        std::unique_ptr<Error> cause_ = nullptr;

        Error(
            std::error_code ec,
            std::string msg = "",
            std::source_location loc = std::source_location::current()
        ) : ec(ec), loc(loc), message_(std::move(msg)) {}

        template<typename EnumT>
        requires std::is_error_code_enum_v<EnumT>
        Error(
            EnumT e,
            std::string msg = "",
            std::source_location l = std::source_location::current()
        ) : ec(std::make_error_code(e)), loc(l), message_(std::move(msg)) {}

        Error(const Error &other) : ec(other.ec), loc(other.loc), message_(other.message_) {
            if (other.cause_) {
                cause_ = std::make_unique<Error>(*other.cause_);
            }
        }

        Error &operator=(const Error &other) {
            if (this != &other) {
                ec = other.ec;
                loc = other.loc;
                message_ = other.message_;
                if (other.cause_) {
                    cause_ = std::make_unique<Error>(*other.cause_);
                } else {
                    cause_.reset();
                }
            }
            return *this;
        }

        Error(Error &&) noexcept = default;
        Error &operator=(Error &&) noexcept = default;
        ~Error() = default;

        auto with_cause(const Error &cause) -> Error & {
            cause_ = std::make_unique<Error>(cause);
            return *this;
        }

        auto with_cause(Error &&cause) -> Error & {
            cause_ = std::make_unique<Error>(std::move(cause));
            return *this;
        }

        auto cause() const -> const Error * {
            return cause_.get();
        }

        explicit operator bool() const {
            return ec.operator bool();
        }

        auto operator==(const Error &other) const -> bool {
            return ec == other.ec;
        }

        [[nodiscard]]
        auto what() const -> std::string {
            if (message_.empty()) {
                return ec.message();
            }
            return ec.message() + ": " + message_;
        }
    };

    template<typename T>
    using Result = std::expected<Regularized<T>, Error>;


    auto make_err_result(
        std::errc errc,
        std::string msg = "",
        std::source_location loc = std::source_location::current()
    ) -> std::unexpected<Error> {
        return std::unexpected{Error{std::make_error_code(errc), std::move(msg), loc}};
    }
}

template <>
struct std::formatter<ppnt::Error> {
    static constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    static constexpr auto extract_filename(const char *path) -> std::string_view {
        std::string_view p(path);
        auto pos = p.find_last_of("/\\");
        return (pos != std::string_view::npos) ? p.substr(pos + 1) : p;
    }

    static constexpr auto extract_project_path(std::string_view p) -> std::string_view {

        size_t src_pos = p.find("/src/");
        if (src_pos == std::string_view::npos) {
            src_pos = p.find("\\src\\");
        }
        if (src_pos != std::string_view::npos) {

            if (src_pos > 0) {
                size_t parent_slash = p.rfind('/', src_pos - 1);
                if (parent_slash == std::string_view::npos) {
                    parent_slash = p.rfind('\\', src_pos - 1);
                }

                if (parent_slash != std::string_view::npos) {
                    return p.substr(parent_slash + 1);
                }
            }
            return p.substr(src_pos + 1);
        }
        return p;
    }

    auto format(const ppnt::Error &root_err, std::format_context &ctx) const {
        auto out = ctx.out();
        const ppnt::Error *curr = &root_err;
        bool is_first = true;

        while (curr) {
            if (is_first) {
                out = std::format_to(out, "\n");
            } else {
                out = std::format_to(out, "\n  Caused by: ");
            }

            std::string_view filename = extract_project_path(curr->loc.file_name());

            out = std::format_to(out, "[{}:{}] {} @ {}:{}",
                curr->ec.category().name(),
                curr->ec.value(),
                curr->what(),
                filename,
                curr->loc.line());

            curr = curr->cause();
            is_first = false;
        }
        return out;
    }
};

template<>
struct std::formatter<std::monostate> {
    static constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const std::monostate &ms, std::format_context &ctx) const  {
        auto out = ctx.out();
        out = std::format_to(out, "Unit", ms);
        return out;
    }
};
