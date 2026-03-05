export module ppnt.http.http_types:header;

import std;
import ppnt.common;
// import :version;

export namespace ppnt::http {

    class HttpHeader {
    public:
        std::string name;
        std::string value;

        HttpHeader() : name(), value() {}

        HttpHeader(std::string name, std::string value) : name(std::move(name)), value(std::move(value)) {}
        HttpHeader(std::string_view name, std::string_view value) : name(name), value(value) {}

        auto operator==(const HttpHeader &other) const -> bool {
            return ignore_case_equal(name, other.name) && value == other.value;
        }

        template<std::size_t N>
        [[nodiscard]]
        auto get() const -> const std::string & {
            if constexpr (N == 0) return (name);
            else if constexpr (N == 1) return (value);
            else static_assert(false);
        }
    };

    class HttpHeaderList {
    private:
        std::vector<HttpHeader> headers_;
    public:
        HttpHeaderList() : headers_() {}
        HttpHeaderList(std::initializer_list<HttpHeader> headers) : headers_(headers) {}
        HttpHeaderList(HttpHeaderList &&other) noexcept : headers_(std::move(other.headers_)) {
        }
        HttpHeaderList(const HttpHeaderList &other) = default;

        auto operator=(HttpHeaderList &&other) noexcept -> HttpHeaderList & {
            if (this != &other) {
                headers_ = std::move(other.headers_);
            }
            return *this;
        }

        auto operator==(const HttpHeaderList &other) const -> bool {
            return headers_ == other.headers_;
        }

        auto add(std::string_view name, std::string_view value) -> void {
            headers_.emplace_back(name, value);
        }

        auto add(HttpHeader header) -> void {
            headers_.emplace_back(std::move(header));
        }

        auto set(std::string_view name, std::string_view value) -> void {
            std::erase_if(headers_, [&] (const HttpHeader &h) {
                return ignore_case_equal(h.name, name);
            });
            add(name, value);
        }

        [[nodiscard]]
        auto get(std::string_view name) const -> std::optional<std::string_view> {
            for (const auto &header : headers_) {
                if (ignore_case_equal(header.name, name)) {
                    return header.value;
                }
            }
            return std::nullopt;
        }


        [[nodiscard]]
        auto contains(std::string_view name) const -> bool {
            return get(name).has_value();
        }

        auto begin(this auto &&self) {
            return self.headers_.begin();
        }

        auto end(this auto &&self) {
            return self.headers_.end();
        }

        auto clear() -> void {
            headers_.clear();
        }

        auto empty() const -> bool {
            return headers_.empty();
        }

        auto size() -> std::size_t {
            return headers_.size();
        }

        auto reserve(std::size_t size) -> void {
            headers_.reserve(size);
        }

        auto to_string() const -> std::string {
            std::string result;
            for (const auto &header : headers_) {
                result += std::format("{}:{}\n", header.name, header.value);
            }
            return result;
        }
    };
}

template<>
struct std::tuple_size<ppnt::http::HttpHeader> : std::integral_constant<std::size_t, 2> {};

template<std::size_t N>
struct std::tuple_element<N, ppnt::http::HttpHeader> {
    using type = std::string_view;
};