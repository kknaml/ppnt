export module ppnt.net.tcp_stream;

import std;
import ppnt.common;
import ppnt.libc;
import ppnt.net.addr;
import ppnt.io.task;
import ppnt.io.ops;

inline  auto close_fd(int fd) noexcept {
    if (fd != -1) ppnt::libc::close(fd);
}

export namespace ppnt::net {


    class TcpStream: public NonCopy{
    private:
        int fd_;
    public:
        TcpStream() noexcept : fd_(-1) {}
        explicit TcpStream(int fd) noexcept : fd_(fd) {}

        TcpStream(TcpStream &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
        auto operator=(TcpStream &&other) noexcept -> TcpStream& {
            if (this != &other) {
                close_fd(this->fd_);
                this->fd_ = std::exchange(other.fd_, -1);
            }
            return *this;
        }

        static auto connect(SocketAddress addr, uint32_t timeout_ms = 0)-> io::Task<Result<TcpStream>>  {
            auto fd = libc::socket(addr.family(), libc::sock_stream | libc::sock_nonblock | libc::sock_cloexec, 0);
            if (fd < 0) {
                co_return std::unexpected{Error{std::error_code(libc::error_no(), std::system_category())}};
            }
            log::info({"connecting to {}"}, addr);
            auto res = co_await io::async_connect(fd, addr.sockaddr_ptr(), addr.socklen(), timeout_ms);
            if (!res) {
                close_fd(fd);
                auto &cause = res.error();
                auto err = Error{cause.ec, std::format("cannot connect to {}", addr)};
                err.with_cause(std::move(cause));
                co_return std::unexpected{err};
            }
            co_return TcpStream(fd);
        }

        static auto connect(std::string_view host, int port, uint32_t timeout_ms = 0) -> io::Task<Result<TcpStream>> {
            auto addrs = resolve_host(host, port);
            if (!addrs) co_return std::unexpected{addrs.error()};
            if (addrs->empty()) {
                co_return std::unexpected{
                    Error{
                        std::make_error_code(std::errc::destination_address_required),
                        std::format("failed to connect to {}:{}", host, port)
                    }
                };
            }
            co_return co_await TcpStream::connect(addrs->front(), timeout_ms);
        }

        auto read(std::span<uint8_t> buf, uint32_t timeout_ms = 0) const {
            return io::async_read(fd_, buf.data(), buf.size(), -1, timeout_ms);
        }

        auto read_bs(int nbytes, uint32_t flags = 0, uint32_t timeout_ms = 0) const {
            return io::async_recv_bs(fd_, nbytes, flags, timeout_ms);
        }

        auto write(std::span<const uint8_t> buf, uint32_t timeout_ms = 0) const {
            return io::async_write(fd_, buf.data(), buf.size(), -1, timeout_ms);
        }

        auto read_exact(std::span<uint8_t> buf) -> io::Task<Result<Unit>> {
            size_t total_read = 0;
            while (total_read < buf.size()) {
                auto res = co_await this->read(buf.subspan(total_read));
                if (!res) co_return std::unexpected{res.error()};
                auto n = *res;
                if (n == 0) {
                    // EOF but not full filled
                    co_return std::unexpected{Error{std::make_error_code(std::errc::io_error), "read_exact EOF"}};
                }
                total_read += n;
            }
            co_return {};
        }

        auto write_all(std::span<const uint8_t> buf, uint32_t timeout_ms = 0) const -> io::Task<Result<Unit>> {
            size_t total_written = 0;
            while (total_written < buf.size()) {
                auto res = co_await this->write(buf.subspan(total_written), timeout_ms);
                if (!res) co_return std::unexpected{res.error()};
                auto n = *res;
                if (n == 0) {
                    co_return std::unexpected{Error{std::make_error_code(std::errc::broken_pipe), "write_all broken pipe"}};
                }
                total_written += n;
            }
            co_return {};
        }

        [[nodiscard]]
        auto native_handle() const noexcept -> int {
            return fd_;
        }

        [[nodiscard]]
        auto leak_handle() noexcept -> int {
            return std::exchange(fd_, -1);
        }

        auto close() {
            close_fd(fd_);
            fd_ = -1;
        }

        auto is_alive() const -> bool {
            // TODO
            return fd_ >= 0;
        }
    };
}
