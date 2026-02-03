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
        liburing::io_uring_prep_read(sqe, ev_fd_, &ev_buf_, sizeof(ev_buf_), 0);
        liburing::io_uring_sqe_set_data(sqe, &ev_op_);
    }

    auto Ring::run_one() -> void {
        liburing::io_uring_submit_and_wait(&ring_, 1);

        liburing::io_uring_cqe* cqe;
        unsigned head;
        unsigned count = 0;
        auto need_rearm = false;

        // Process all completed events
        // io_uring_for_each_cqe macro replacement
        for (liburing::io_uring_cqe_iter iter = liburing::io_uring_cqe_iter_init(&ring_);
            liburing::io_uring_cqe_iter_next(&iter, &cqe);) {

            count++;
            auto *op = static_cast<Operation *>(liburing::io_uring_cqe_get_data(cqe));
            if (op == &ev_op_) [[unlikely]] {
                need_rearm = true;
            } else if (op) [[likely]] {
                op->result = cqe->res;
                if (op->handle) op->handle.resume();
            }
            }
        liburing::io_uring_cq_advance(&ring_, count);

        if (need_rearm) {
            run_external_tasks();
            arm_eventfd_read();
        }
    }

    auto Ring::post(std::coroutine_handle<> h) -> void {
        {
            std::lock_guard lock(mtx_);
            external_tasks_.push_back(h);
        }
        if (!notified_.exchange(true)) {
            static const uint64_t buf = 1;
            libc::write(ev_fd_, &buf, sizeof(buf));
        }
    }

    auto Ring::run_external_tasks() -> void {
        notified_.store(false);
        std::deque<std::coroutine_handle<>> tasks;
        {
            std::lock_guard lock{mtx_};
            tasks.swap(external_tasks_);
        }
        for (auto h : tasks) {
            if (h && !h.done()) h();
        }
    }
}
