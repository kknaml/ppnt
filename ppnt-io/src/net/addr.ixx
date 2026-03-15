

export module ppnt.net.addr;

import std;
import ppnt.libc;
import ppnt.err;


export namespace ppnt::net {
    class SocketAddress {
    private:
        libc::sockaddr_storage storage_;
    public:
        SocketAddress() {
            storage_ = {};
            storage_.ss_family = libc::af_unspec;
        }

        SocketAddress(const libc::sockaddr *addr, libc::socklen_t len) {
            std::memcpy(&storage_, addr, len);
        }

        static auto from_ip_port(std::string_view ip, int port) -> Result<SocketAddress> {
            SocketAddress addr;
            port = libc::htons(port);
            auto *addr4 = reinterpret_cast<libc::sockaddr_in *>(&addr);
            if (libc::inet_pton(libc::af_inet, ip.data(), &addr4->sin_addr) == 1) {
                addr4->sin_family = libc::af_inet;
                addr4->sin_port = port;
                return addr;
            }
            auto *addr6 = reinterpret_cast<libc::sockaddr_in6 *>(&addr);
            if (libc::inet_pton(libc::af_inet6, ip.data(), &addr6->sin6_addr) == 1) {
                addr6->sin6_family = libc::af_inet6;
                addr6->sin6_port = port;
                return addr;
            }
            return std::unexpected(
                Error{std::make_error_code(std::errc::invalid_argument), std::format("invalid ip: {}", ip)}
            );
        }

        [[nodiscard]]
        auto family() const noexcept -> libc::sa_family_t {
            return storage_.ss_family;
        }

        [[nodiscard]]
        auto is_v4() const noexcept -> bool {
            return storage_.ss_family == libc::af_inet;
        }

        [[nodiscard]]
        auto is_v6() const noexcept -> bool {
            return storage_.ss_family == libc::af_inet6;
        }

        [[nodiscard]]
        auto sockaddr_ptr() const noexcept -> const libc::sockaddr * {
            return reinterpret_cast<const libc::sockaddr *>(&storage_);
        }

        [[nodiscard]]
        auto sockaddr_ptr() noexcept -> libc::sockaddr * {
            return reinterpret_cast<libc::sockaddr *>(&storage_);
        }

        [[nodiscard]]
        auto socklen() const noexcept -> libc::socklen_t {
            if (is_v4()) return sizeof(libc::sockaddr_in);
            if (is_v6()) return sizeof(libc::sockaddr_in6);
            return 0;
        }

        [[nodiscard]]
        auto port() const noexcept -> int {
            if (is_v4()) {
                auto *p = reinterpret_cast<const libc::sockaddr_in *>(&storage_);
                return libc::ntohs(p->sin_port);
            }
            if (is_v6()) {
                auto *p = reinterpret_cast<const libc::sockaddr_in6 *>(&storage_);
                return libc::ntohs(p->sin6_port);
            }
            return 0;
        }

        auto set_port(int port) noexcept -> void {
            if (is_v4()) {
                auto *p = reinterpret_cast<libc::sockaddr_in *>(&storage_);
                p->sin_port = libc::htons(port);
            } else if (is_v6()) {
                auto *p = reinterpret_cast<libc::sockaddr_in6 *>(&storage_);
                p->sin6_port = libc::htons(port);
            }
        }

        [[nodiscard]]
        auto to_ip_string() const -> std::string {
            char buf[libc::inet6_addrstrlen] = {};
            const void *src = nullptr;
            if (is_v4()) {
                src = &reinterpret_cast<const libc::sockaddr_in *>(&storage_)->sin_addr;
            } else if (is_v6()) {
                src = &reinterpret_cast<const libc::sockaddr_in6 *>(&storage_)->sin6_addr;
            } else {
                src = "unknown";
            }
            if (libc::inet_ntop(storage_.ss_family, src, buf, sizeof(buf))) {
                return std::string{buf};
            }
            return "invalid";
        }

        [[nodiscard]]
        auto to_string() const -> std::string {
            if (is_v6()) {
                return std::format("[{}]:{}", to_ip_string(), port());
            }
            return std::format("{}:{}", to_ip_string(), port());
        }
    };


    template<typename T>
    concept HostResolver = requires(std::string_view host, int port) {
        { T::resolve(host, port) } -> std::same_as<Result<std::vector<SocketAddress>>>;
    };

    namespace detail {
        auto map_getaddrinfo_err(int err, std::string msg) -> Error;
    }

    struct LinuxGlibcResolver {
        static auto resolve(std::string_view host, int port) -> Result<std::vector<SocketAddress>> {
            libc::addrinfo hints{}, *res = nullptr;
            std::memset(&hints, 0, sizeof(hints));
            hints.ai_family = libc::af_unspec;
            hints.ai_socktype = libc::sock_stream;

            std::string host_str(host);
            std::string port_str = std::to_string(port);

            int err = libc::getaddrinfo(host_str.c_str(), port_str.c_str(), &hints, &res);
            if (err != 0) {
                // if (err == EAI_SYSTEM) {
                //     return std::unexpected(std::error_code(errno, std::system_category()));
                // }
                // return std::unexpected(std::make_error_code(std::errc::address_not_available));
                // return std::unexpected(Error{std::error_code(-err, std::system_category())});
                return std::unexpected(detail::map_getaddrinfo_err(err, std::move(host_str)));
            }

            std::vector<SocketAddress> results;
            for (libc::addrinfo *ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
                results.emplace_back(ptr->ai_addr, static_cast<libc::socklen_t>(ptr->ai_addrlen));
            }

            libc::freeaddrinfo(res);
            return results;
        }
    };

    template<HostResolver Resolver = LinuxGlibcResolver>
    auto resolve_host(std::string_view host, int port) -> Result<std::vector<SocketAddress>> {
        return Resolver::resolve(host, port);
    }

    template<HostResolver Resolver = LinuxGlibcResolver>
    auto resolve_first(std::string_view host, int port) -> Result<SocketAddress> {
        auto res = Resolver::resolve(host, port);
        if (!res) return std::unexpected(res.error());
        if (res->empty()) return std::unexpected(Error{std::make_error_code(std::errc::destination_address_required)});
        return res->front();
    }
}
