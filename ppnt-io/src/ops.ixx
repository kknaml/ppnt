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

    template<typename PrepFn>
    requires std::is_invocable_v<PrepFn, liburing::io_uring_sqe *>
    struct AsyncOp {
        Operation operation_{};
        PrepFn prep_;

        explicit(false) AsyncOp(PrepFn &&prep) : prep_(std::move(prep)) {}

        auto await_ready() const noexcept -> bool { return false; }

        auto await_suspend(std::coroutine_handle<> h) -> void {
            operation_.handle = h;
            Ring::current().submit(operation_, prep_);
        }

        auto await_resume() const noexcept -> int {
            return operation_.result;
        }
    };

    template<typename PrepFn>
    AsyncOp(PrepFn) -> AsyncOp<PrepFn>;

    template<typename Inner, typename AwaitResume>
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


    export auto async_read_raw(int fd, void *buf, unsigned nbytes, uint64_t offset = -1) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_read(sqe, fd, buf, nbytes, offset);
        }};
    }

    export auto async_read(int fd, void *buf, unsigned nbytes, uint64_t offset = -1) {
        return DelegateOpAwaiter(async_read_raw(fd, buf, nbytes, offset), detail::map_async_read_result);
    }


    export auto async_write_raw(int fd, const void *buf, unsigned nbytes, uint64_t offset = -1) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_write(sqe, fd, buf, nbytes, offset);
        }};
    }

    export auto async_write(int fd, const void *buf, unsigned nbytes, uint64_t offset = -1) {
        return DelegateOpAwaiter(async_write_raw(fd, buf, nbytes, offset), detail::map_async_write_result);
    }

    export auto async_close_raw(int fd) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_close(sqe, fd);
        }};
    }

    export auto async_connect_raw(int fd, const libc::sockaddr *addr, libc::socklen_t addrlen) {
        return AsyncOp{[=](liburing::io_uring_sqe *sqe) {
            liburing::io_uring_prep_connect(sqe, fd, addr, addrlen);
        }};
    }

    export auto async_connect(int fd, const libc::sockaddr *addr, libc::socklen_t addrlen) {
        return DelegateOpAwaiter(async_connect_raw(fd, addr, addrlen), detail::map_async_connect_result);
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
