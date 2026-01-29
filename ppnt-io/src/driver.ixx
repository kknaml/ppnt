export module ppnt.io.driver;

import std;
import ppnt.log;
import liburing;

namespace ppnt::io {

    export struct Operation {
        std::coroutine_handle<> handle{nullptr};
        std::int32_t result{0};
    };

    export class Ring {

    public:
        Ring(unsigned entries = 1024) {
             if (int ret = liburing::io_uring_queue_init(entries, &ring_, 0); ret < 0) {
                 std::terminate(); 
             }
        }

        ~Ring() {
            liburing::io_uring_queue_exit(&ring_);
        }

        static auto set_ring(Ring *ring) -> void;
        static auto current() -> Ring &;

        // Generic submit
        template<typename PrepFn>
        void submit(Operation &op, PrepFn &&prep) {
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
        
        void run_one() {
             liburing::io_uring_submit_and_wait(&ring_, 1);
             
             liburing::io_uring_cqe* cqe;
             unsigned head;
             unsigned count = 0;

             // Process all completed events
             // io_uring_for_each_cqe macro replacement
             for (liburing::io_uring_cqe_iter iter = liburing::io_uring_cqe_iter_init(&ring_);
                  liburing::io_uring_cqe_iter_next(&iter, &cqe);) {
                 count++;
                 auto* op = static_cast<Operation*>(liburing::io_uring_cqe_get_data(cqe));
                 if (op) {
                     op->result = cqe->res;
                     if (op->handle) op->handle.resume();
                 }
             }
             liburing::io_uring_cq_advance(&ring_, count);
        }

    private:
        liburing::io_uring ring_;
    };



}
