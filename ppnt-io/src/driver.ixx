export module ppnt.io.driver;

import std;
import ppnt.log;
import liburing;
import ppnt.libc;
import ppnt.traits;

namespace ppnt::io {

    export struct alignas(8) Operation {
        std::coroutine_handle<> handle{nullptr}; // 8 bytes

        struct {
            int32_t result{0};           // 4 bytes: CQE result
            uint16_t bid{0};             // 2 bytes: Buffer ID
            uint8_t ref_count{1};        // 1 byte: For linked timeout (IO + Timeout)
            uint8_t flags{0};            // 1 byte: Status flags
        } data;

        // Union for space optimization
        union {
            uint32_t timeout_ms;         // User input (before submit)
            liburing::__kernel_timespec ts; // Kernel usage (during submit)
        };

        // FLAGS
        static constexpr uint8_t F_HAS_BUFFER = 1 << 0;
        static constexpr uint8_t F_IS_LINKED  = 1 << 1;

        auto reset(uint32_t t_ms = 0) -> void {
            handle = nullptr;
            data.result = 0;
            data.bid = 0;
            data.ref_count = 1;
            data.flags = 0;
            timeout_ms = t_ms;
        }
    };

    export class Ring : public NonCopy {

    public:
        Ring(unsigned entries = 1024);

        ~Ring();

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

            if (op.timeout_ms > 0) {
                sqe->flags |= liburing::iosqe_io_link;
                auto *ts_sqe = liburing::io_uring_get_sqe(&ring_);
                auto ms = op.timeout_ms;
                op.ts.tv_sec = ms / 1000;
                op.ts.tv_nsec = (ms % 1000) * 1000000;

                op.data.ref_count = 2; // Expect 2 CQEs (IO result + Timeout result)
                op.data.flags |= Operation::F_IS_LINKED;

                liburing::io_uring_prep_link_timeout(ts_sqe, &op.ts, 0);
                liburing::io_uring_sqe_set_data(ts_sqe, reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(&op) | 1));
            }
        }
        
        auto run_one() -> void;

        auto post(std::coroutine_handle<> h) -> void;

        auto get_buffer(uint16_t bid) const -> void *;
        auto recycle_buffer(uint16_t bid) -> void;

    private:
        auto arm_eventfd_read() -> void;
        auto run_external_tasks() -> void;
        auto setup_buffers() -> void;


    private:
        liburing::io_uring ring_{};
        int ev_fd_{};
        uint64_t ev_buf_{};
        Operation ev_op_{};
        std::mutex mtx_{};
        std::atomic<bool> notified_{false};
        std::deque<std::coroutine_handle<>> external_tasks_{};

        // Buffer Management internals
        void* buffer_mem_{nullptr};         // The actual memory block
        liburing::io_uring_buf_ring *buf_ring_{nullptr}; // 5.19+ Ring structure
        bool using_buf_ring_{false};        // Flag for mode selection

    public:
        static constexpr uint16_t BGID = 0;
        static constexpr uint32_t BUF_SIZE = 4096;
        static constexpr uint32_t BUF_COUNT = 512;
    };


}
