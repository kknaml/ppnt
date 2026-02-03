export module ppnt.io.driver;

import std;
import ppnt.log;
import liburing;
import ppnt.libc;
import ppnt.traits;

namespace ppnt::io {

    export struct Operation {
        std::coroutine_handle<> handle{nullptr};
        std::int32_t result{0};
    };

    export class Ring : public NonCopy {

    public:
        Ring(unsigned entries = 1024) {
             if (int ret = liburing::io_uring_queue_init(entries, &ring_, 0); ret < 0) {
                 std::terminate(); 
             }
            ev_fd_ = libc::eventfd(0, libc::efd_nonblock | libc::efd_cloexec);
            arm_eventfd_read();
        }

        ~Ring() {
            liburing::io_uring_queue_exit(&ring_);
        }

        static auto set_ring(Ring *ring) -> void;
        static auto current() -> Ring &;

        // Generic submit
        template<typename PrepFn>
        auto submit(Operation &op, PrepFn &&prep) -> void {
            auto *sqe = liburing::io_uring_get_sqe(&ring_);
            if (!sqe) {
                // simple handle: submit and retry
                liburing::io_uring_submit(&ring_);
                sqe = liburing::io_uring_get_sqe(&ring_);
                if (!sqe) [[unlikely]] {
                    log::error({"io_uring_get_sqe failed"});
                    std::terminate(); // Should not happen if correctly managed
                }
            }
            
            prep(sqe);
            liburing::io_uring_sqe_set_data(sqe, &op);
        }
        
        auto run_one() -> void {
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

        auto post(std::coroutine_handle<> h) -> void {
            {
                std::lock_guard lock(mtx_);
                external_tasks_.push_back(h);
            }
            if (!notified_.exchange(true)) {
                libc::write(ev_fd_, &ev_buf_, sizeof(ev_buf_));
            }
        }

    private:
        auto arm_eventfd_read() -> void;

        auto run_external_tasks() -> void {
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

    private:
        liburing::io_uring ring_{};
        int ev_fd_{};
        uint64_t ev_buf_{};
        Operation ev_op_{};
        std::mutex mtx_{};
        std::atomic<bool> notified_{false};
        std::deque<std::coroutine_handle<>> external_tasks_{};
    };


}
