module ppnt.io.driver;

import ppnt.log;

namespace ppnt::io {

    thread_local Ring *current_ring = nullptr;

    auto Ring::set_ring(Ring *ring) -> void {
        current_ring = ring;
    }

    auto Ring::current() -> Ring & {
        if (!current_ring) {
            // Avoid including unnecessary headers or using complex logging if not needed
            // But we can include <exception> or just use builtin trap if we want minimal dep
            // For now std::terminate is fine.
            log::error({"current_ring not set"});
            std::terminate();
        }
        return *current_ring;
    }

    auto Ring::arm_eventfd_read() -> void {
        auto *sqe = liburing::io_uring_get_sqe(&ring_);
        if (!sqe) {
            liburing::io_uring_submit(&ring_);
            sqe = liburing::io_uring_get_sqe(&ring_);
            if (!sqe) [[unlikely]] {
                log::error({"io_uring_get_sqe failed"});
                std::terminate();
            }
        }
        liburing::io_uring_prep_read(sqe, ev_fd_, &ev_buf_, sizeof(ev_fd_), 0);
        liburing::io_uring_sqe_set_data(sqe, &ev_op_);
    }
}
