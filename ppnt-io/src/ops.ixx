export module ppnt.io.ops;

import std;
import ppnt.io.driver;
import liburing;
import ppnt.traits;
import ppnt.libc;
import ppnt.err;

namespace ppnt::io {

    namespace detail {
        auto map_async_read_result(int result) -> Result<int>;
        auto map_async_write_result(int result) -> Result<int>;
        auto map_async_connect_result(int result) -> Result<Unit>;
    }

    // for buffer select
    export struct [[nodiscard]] IOResult : public NonCopy {
        int result{};
        void *buffer{};
        uint16_t bid{};

        IOResult() = default;

        IOResult(int res, void* buf, uint16_t id)
            : result(res), buffer(buf), bid(id) {}

        IOResult(IOResult &&other) noexcept
        : result(other.result), buffer(std::exchange(other.buffer, nullptr)), bid(other.bid) {}

        auto operator=(IOResult &&other) noexcept -> IOResult & {
            if (this != &other) {
                if (buffer) {
                    Ring::current().recycle_buffer(bid);
                }
                result = other.result;
                buffer = std::exchange(other.buffer, nullptr);
                bid = other.bid;
            }
            return *this;
        }

        ~IOResult() {
            if (buffer) {
                Ring::current().recycle_buffer(bid);
                buffer = nullptr;
            }
        }

        explicit operator bool() const noexcept {
            return result >= 0;
        }

        [[nodiscard]]
        auto error() const -> std::unexpected<Error> {
            return std::unexpected{Error{std::error_code(-this->result, std::generic_category())}};
        }

        [[nodiscard]]
        auto data() const -> std::span<uint8_t> {
            if (result <= 0 || !buffer) return {};
            return {static_cast<uint8_t*>(buffer), static_cast<size_t>(result)};
        }
    };

    export template<typename PrepFn>
    requires std::is_invocable_v<PrepFn, liburing::io_uring_sqe *>
    struct AsyncOp {
        Operation operation_{};
        PrepFn prep_;

        explicit(false) AsyncOp(PrepFn &&prep, uint32_t timeout_ms = 0) : prep_(std::move(prep)) {
            operation_.reset(timeout_ms);
        }

        auto await_ready() const noexcept -> bool { return false; }

        auto await_suspend(std::coroutine_handle<> h) -> void {
            operation_.handle = h;
            Ring::current().submit(operation_, prep_);
        }

        auto await_resume() const noexcept -> int {
            return operation_.data.result;
        }
    };

    template<typename PrepFn>
    AsyncOp(PrepFn) -> AsyncOp<PrepFn>;

    template<typename Fn>
    auto make_op(Fn &&fn, uint32_t timeout_ms = 0) {
        return AsyncOp<std::decay_t<Fn>>(std::forward<Fn>(fn), timeout_ms);
    }

    export template<typename PrepFn>
    struct AsyncBufferOp {
        Operation operation_{};
        [[no_unique_address]] PrepFn prep_;

        template<typename Fn>
        explicit AsyncBufferOp(Fn &&fn, uint32_t timeout_ms = 0)
            : prep_(std::forward<Fn>(fn)) {
            operation_.reset(timeout_ms);
        }

        auto await_ready() const noexcept -> bool { return false; }

        auto await_suspend(std::coroutine_handle<> h) -> void {
            operation_.handle = h;
            Ring::current().submit(operation_, prep_);
        }

        auto await_resume() -> IOResult {
            int res = operation_.data.result;
            void *ptr = nullptr;
            uint16_t bid = 0;

            if (res >= 0 && (operation_.data.flags & Operation::F_HAS_BUFFER)) {
                bid = operation_.data.bid;
                ptr = Ring::current().get_buffer(bid);
            }

            return IOResult{res, ptr, bid};
        }
    };

    template<typename PrepFn>
    AsyncBufferOp(PrepFn) -> AsyncBufferOp<PrepFn>;

    template<typename Fn>
    auto make_buffer_op(Fn &&fn, uint32_t timeout_ms = 0) {
        return AsyncBufferOp<std::decay_t<Fn>>(std::forward<Fn>(fn), timeout_ms);
    }

    export template<typename Inner, typename AwaitResume>
    struct DelegateOpAwaiter {
        Inner inner_;
        [[no_unique_address]]
        AwaitResume await_resume_;
        DelegateOpAwaiter(Inner &&inner, AwaitResume &&f) : inner_(std::move(inner)), await_resume_(std::move(f)) {}

        [[nodiscard]]
        auto await_ready() const noexcept -> bool { return inner_.await_ready(); }
        auto await_suspend(std::coroutine_handle<> h) { return inner_.await_suspend(h); }
        auto await_resume() const noexcept  { return await_resume_(inner_.await_resume()); }
    };

    template<typename Inner, typename AwaitResume>
    DelegateOpAwaiter(Inner, AwaitResume) -> DelegateOpAwaiter<Inner, AwaitResume>;


    export auto async_read_raw(int fd, void *buf, unsigned nbytes, uint64_t offset = -1, uint32_t timeout_ms = 0) {
        return make_op([=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        }, timeout_ms);
    }

    export auto async_read(int fd, void *buf, unsigned nbytes, uint64_t offset = -1, uint32_t timeout_ms = 0) {
        return DelegateOpAwaiter(async_read_raw(fd, buf, nbytes, offset, timeout_ms), detail::map_async_read_result);
    }

    export auto async_recv_bs(int fd, uint32_t nbytes, uint32_t flags = 0, uint32_t timeout_ms = 0) {
        return make_buffer_op([=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_recv(sqe, fd, nullptr, nbytes, flags);
            sqe->flags |= liburing::iosqe_buffer_select;
            sqe->buf_group = Ring::BGID;
        }, timeout_ms);
    }


    export auto async_write_raw(int fd, const void *buf, unsigned nbytes, uint64_t offset = -1, uint32_t timeout_ms = 0) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        }, timeout_ms};
    }

    export auto async_write(int fd, const void *buf, unsigned nbytes, uint64_t offset = -1, uint32_t timeout_ms = 0) {
        return DelegateOpAwaiter(async_write_raw(fd, buf, nbytes, offset, timeout_ms), detail::map_async_write_result);
    }

    export auto async_close_raw(int fd) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_close(sqe, fd);
        }};
    }

    export auto async_connect_raw(int fd, const libc::sockaddr *addr, libc::socklen_t addrlen, uint32_t timeout_ms = 0) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_connect(sqe, fd, addr, addrlen);
        }, timeout_ms};
    }

    export auto async_connect(int fd, const libc::sockaddr *addr, libc::socklen_t addrlen, uint32_t timeout_ms = 0) {
        return DelegateOpAwaiter(async_connect_raw(fd, addr, addrlen, timeout_ms), detail::map_async_connect_result);
    }

    //
    export auto async_accept(int fd, libc::sockaddr *addr, libc::socklen_t *addrlen, int flags = 0) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
        }};
    }

    export auto async_timeout_raw(const liburing::__kernel_timespec *ts, unsigned count, unsigned flags) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
           liburing::io_uring_prep_timeout(sqe, ts, count, flags);
        }};
    }

    export auto delay(liburing::__kernel_timespec ts) {
        return DelegateOpAwaiter(AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_timeout(sqe, &ts, 0, 0);
        }}, [](int ret) -> Result<Unit> {
            if (ret == -libc::e_time) {
                return {};
            }
            if (ret == -libc::e_canceled) {
                return make_err_result(std::errc::operation_canceled, "delay canceled");
            }
            return make_err_result(std::errc::no_message, std::format("delay failed: {}", ret));
        });
    }

    export auto delay(long ms) {
        liburing::__kernel_timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000;
        return delay(ts);
    }

}
