module ppnt.net.url;

namespace ppnt::net {
    auto Url::to_string() const -> std::string {
        return href();
    }

    auto Url::parse_authority(std::string_view authority, Url &url) -> Result<Unit> {
        auto at_pos = authority.find('@');
        if (at_pos != std::string_view::npos) {
            url.user_info_ = std::string(authority.substr(0, at_pos));
            authority.remove_prefix(at_pos + 1);
        }

        auto colon_pos = authority.rfind(':');
        auto close_bracket = authority.rfind(']');

        bool has_port = false;
        if (colon_pos != std::string_view::npos) {
            if (close_bracket == std::string_view::npos || colon_pos > close_bracket) {
                has_port = true;
            }
        }

        if (has_port) {
            auto port_str = authority.substr(colon_pos + 1);
            int port_num = 0;
            auto [ptr, ec] = std::from_chars(port_str.data(), port_str.data() + port_str.size(), port_num);
            if (ec != std::errc()) {
                return make_err_result(std::errc::invalid_argument, "InvalidPort");
            }
            url.port_ = port_num;
            url.host_ = to_lower_ascii_str(authority.substr(0, colon_pos));
        } else {
            url.port_ = -1;
            url.host_ = to_lower_ascii_str(authority);
        }

        if (url.host_.size() >= 2 && url.host_.front() == '[' && url.host_.back() == ']') {
            url.host_ = url.host_.substr(1, url.host_.size() - 2);
        }

        return {};
    }

    namespace {
        auto remove_dot_segments(std::string_view input) -> std::string {
            std::string output;
            output.reserve(input.size());

            while (!input.empty()) {
                if (input.starts_with("../")) {
                    input.remove_prefix(3);
                    continue;
                }
                if (input.starts_with("./")) {
                    input.remove_prefix(2);
                    continue;
                }

                if (input.starts_with("/./")) {
                    input.remove_prefix(2);
                    continue;
                }
                if (input == "/.") {
                    input = "/";
                    continue;
                }

                if (input.starts_with("/../") || input == "/..") {
                    if (input.starts_with("/../")) {
                        input.remove_prefix(3);
                    } else {
                        input = "/";
                    }

                    if (!output.empty()) {
                        auto last_slash = output.rfind('/');
                        if (last_slash == std::string::npos) {
                            output.clear();
                        } else {
                            output.resize(last_slash);
                        }
                    }
                    continue;
                }

                if (input == "." || input == "..") {
                    input = "";
                    continue;
                }

                if (input.starts_with('/')) {
                    size_t next_slash = input.find('/', 1);
                    if (next_slash == std::string_view::npos) {
                        output.append(input);
                        input = "";
                    } else {
                        output.append(input.substr(0, next_slash));
                        input.remove_prefix(next_slash);
                    }
                } else {
                    size_t next_slash = input.find('/');
                    if (next_slash == std::string_view::npos) {
                        output.append(input);
                        input = "";
                    } else {
                        output.append(input.substr(0, next_slash));
                        input.remove_prefix(next_slash);
                    }
                }
            }

            return output;
        }
    }

    auto Url::parse(std::string_view input) -> Result<Url> {
        auto url = Url{};
        if (input.empty()) return make_err_result(std::errc::invalid_argument, "empty url input");
        auto scheme_end = input.find(':');
        if (scheme_end == std::string_view::npos) {
            return make_err_result(std::errc::invalid_argument, "invalid scheme");
        }
        url.scheme_ = to_lower_ascii_str(input.substr(0, scheme_end));
        if (url.scheme_.empty() || !std::isalpha(url.scheme_[0])) {
            return make_err_result(std::errc::invalid_argument, "invalid scheme");
        }
        auto rest = input.substr(scheme_end + 1);
        if (rest.starts_with("//")) {
            rest.remove_prefix(2);
            auto authority_end = rest.find_first_of("/?#");
            auto authority = rest.substr(0, authority_end);
            auto err = parse_authority(authority, url);
            if (!err) {
                return std::unexpected{err.error()};
            }
            if (authority_end != std::string_view::npos) {
                rest = rest.substr(authority_end);
            } else {
                rest = "";
            }
        }
        auto query_pos = rest.find('?');
        auto fragment_pos = rest.find('#');

        if (fragment_pos != std::string_view::npos) {
            url.fragment_ = std::string(rest.substr(fragment_pos + 1));
            if (query_pos > fragment_pos) query_pos = std::string_view::npos;
            rest = rest.substr(0, fragment_pos);
        }

        if (query_pos != std::string_view::npos) {
            url.query_ = std::string(rest.substr(query_pos + 1));
            rest = rest.substr(0, query_pos);
        }

        url.path_ = remove_dot_segments(rest);
        return url;
    }

    auto Url::resolve(std::string_view relative, const Url &base) -> Result<Url> {
        if (relative.empty()) { return base; }

        auto scheme_end = relative.find(':');
        if (scheme_end != std::string_view::npos) {
            auto invalid_chars = relative.substr(0, scheme_end).find_first_of("/?#");
            if (invalid_chars == std::string_view::npos) {
                return parse(relative);
            }
        }

        Url url = base;
        url.query_.clear();
        url.fragment_.clear();

        std::string_view rest = relative;

        auto fragment_pos = rest.find('#');
        if (fragment_pos != std::string_view::npos) {
            url.fragment_ = std::string(rest.substr(fragment_pos + 1));
            rest = rest.substr(0, fragment_pos);
        }

        auto query_pos = rest.find('?');
        if (query_pos != std::string_view::npos) {
            url.query_ = std::string(rest.substr(query_pos + 1));
            rest = rest.substr(0, query_pos);
            if (rest.empty()) return url;
        }

        if (rest.starts_with("//")) {
            rest.remove_prefix(2);
            auto auth_end = rest.find_first_of("/?#");
            std::string_view auth_view = rest.substr(0, auth_end);

            if (auto err = parse_authority(auth_view, url); err) {
                return std::unexpected{err.error()};
            }

            if (auth_end != std::string_view::npos) {
                url.path_ = remove_dot_segments(rest.substr(auth_end));
            } else {
                url.path_ = "/";
            }
        } else if (rest.starts_with('/')) {
            url.path_ = remove_dot_segments(rest);
        } else if (!rest.empty()) {
            if (base.path_.empty()) {
                url.path_ = remove_dot_segments("/" + std::string(rest));
            } else {
                auto last_slash = base.path_.rfind('/');
                std::string base_dir = (last_slash == std::string::npos)
                    ? "/"
                    : base.path_.substr(0, last_slash + 1);
                url.path_ = remove_dot_segments(base_dir + std::string(rest));
            }
        }

        return url;

    }

    auto Url::href() const -> std::string {
        std::string res = scheme_ + "://";
        if (!user_info_.empty()) res += user_info_ + "@";
        if (host_.find(':') != std::string::npos && host_.front() != '[') {
            res += "[" + host_ + "]";
        } else {
            res += host_;
        }

        if (port_ != -1) res += ":" + std::to_string(port_);

        res += path_;
        if (!query_.empty()) res += "?" + query_;
        if (!fragment_.empty()) res += "#" + fragment_;
        return res;
    }

    auto Url::origin() const -> std::string {
        std::string res = scheme_ + "://" + host_;
        if (port_ != -1) res += ":" + std::to_string(port_);
        return res;
    }
}
