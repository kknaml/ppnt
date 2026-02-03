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
        
        auto run_one() -> void;

        auto post(std::coroutine_handle<> h) -> void;

    private:
        auto arm_eventfd_read() -> void;

        auto run_external_tasks() -> void;

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
