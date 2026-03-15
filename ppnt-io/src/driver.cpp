module ppnt.io.driver;

import ppnt.log;

extern "C" int posix_memalign (void **, std::size_t, std::size_t) throw ();

namespace ppnt::io {

    thread_local Ring *current_ring = nullptr;

    Ring::Ring(unsigned entries) {
        if (int ret = liburing::io_uring_queue_init(entries, &ring_, 0); ret < 0) {
            std::terminate();
        }
        setup_buffers();
        ev_fd_ = libc::eventfd(0, libc::efd_nonblock | libc::efd_cloexec);
        arm_eventfd_read();
    }

    Ring::~Ring() {
        if (buffer_mem_) std::free(buffer_mem_);
        if (using_buf_ring_ && buf_ring_) {
            liburing::io_uring_unregister_buf_ring(&ring_, BGID);
            std::free(buf_ring_);
        }
        liburing::io_uring_queue_exit(&ring_);
    }

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

    auto Ring::get_sqe() -> liburing::io_uring_sqe * {
        return liburing::io_uring_get_sqe(&ring_);
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

            auto raw_data = reinterpret_cast<std::uintptr_t>(liburing::io_uring_cqe_get_data(cqe));
            bool is_timeout_cqe = raw_data & 1;
            auto *op = reinterpret_cast<Operation *>(raw_data & ~1ULL);

            if (op == &ev_op_) [[unlikely]] {
                need_rearm = true;
            } else if (op) [[likely]] {
                if (is_timeout_cqe) {
                    if (cqe->res == -libc::e_time) {
                        op->data.result = -libc::e_time;
                    }
                } else {
                    log::info({"cqe wakeup"});
                    op->data.cqe_flags = cqe->flags;
                    if (cqe->res != -libc::e_canceled) {
                        op->data.result = cqe->res;
                        if (cqe->flags & liburing::ioring_cqe_f_buffer) {
                            op->data.bid = static_cast<uint16_t>(cqe->flags >> 16);
                            op->data.flags |= Operation::F_HAS_BUFFER;
                        }
                    }
                }

                if (--op->data.ref_count == 0) {
                    if (op->handle) op->handle();
                }
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

    auto Ring::get_buffer(uint16_t bid) const -> void * {
        return static_cast<uint8_t *>(buffer_mem_) + (bid * BUF_SIZE);
    }

    auto Ring::recycle_buffer(uint16_t bid) -> void {
        void *addr = get_buffer(bid);
        if (using_buf_ring_) {
            // Modern: User-space pointer manipulation, no syscall needed
            liburing::io_uring_buf_ring_add(buf_ring_, addr, BUF_SIZE, bid, BUF_COUNT - 1, 0);
            liburing::io_uring_buf_ring_advance(buf_ring_, 1);
        } else {
            // Legacy: Must issue a new SQE to give buffer back
            auto *sqe = liburing::io_uring_get_sqe(&ring_);
            if (!sqe) {
                // If full, simple handle: submit and wait
                liburing::io_uring_submit(&ring_);
                sqe = liburing::io_uring_get_sqe(&ring_);
            }
            liburing::io_uring_sqe_set_data(sqe, nullptr);
            // Add 1 buffer back, with ID = bid
            liburing::io_uring_prep_provide_buffers(sqe, addr, BUF_SIZE, 1, BGID, bid);
            // Note: We don't force submit here for performance; it will flush on next io
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

    auto Ring::setup_buffers() -> void {
        size_t total_mem_sz = BUF_COUNT * BUF_SIZE;
        buffer_mem_ = std::aligned_alloc(BUF_SIZE, total_mem_sz);
        if (!buffer_mem_) {
            std::terminate();
        }

        // Try 5.19+ IORING_REGISTER_PBUF_RING approach first
        // We do this by attempting the registration.
        auto ring_sz = sizeof(liburing::io_uring_buf) * BUF_COUNT;
        void *ring_ptr{nullptr};
        if (posix_memalign(&ring_ptr, BUF_SIZE, ring_sz) == 0) {
            buf_ring_ = static_cast<liburing::io_uring_buf_ring *>(ring_ptr);
            liburing::io_uring_buf_reg reg{};
            reg.ring_addr = reinterpret_cast<uint64_t>(ring_ptr);
            reg.ring_entries = BUF_COUNT;
            reg.bgid = BGID;

            if (liburing::io_uring_register_buf_ring(&ring_, &reg, 0) == 0) {
                using_buf_ring_ = true;
                liburing::io_uring_buf_ring_init(buf_ring_);
                for (int i = 0; i < BUF_COUNT; i++) {
                    void *addr = static_cast<uint8_t *>(buffer_mem_) + i * BUF_SIZE;
                    liburing::io_uring_buf_ring_add(buf_ring_, addr, BUF_SIZE, i, BUF_COUNT - 1, i);
                }
                liburing::io_uring_buf_ring_advance(buf_ring_, BUF_COUNT);
                log::debug({"Using Modern Buffer Ring (Linux >= 5.19)"});
                return;
            }
            // Registration failed, cleanup and fallback
            std::free(ring_ptr);
            buf_ring_ = nullptr;
        }

        // Fallback: Legacy IORING_OP_PROVIDE_BUFFERS
        using_buf_ring_ = false;
        auto *sqe = liburing::io_uring_get_sqe(&ring_);
        if (!sqe) {
            liburing::io_uring_submit(&ring_);
            sqe = liburing::io_uring_get_sqe(&ring_);
        }
        liburing::io_uring_prep_provide_buffers(sqe, buffer_mem_, BUF_SIZE, BUF_COUNT, BGID, 0);
        liburing::io_uring_submit(&ring_); // Must submit immediately for legacy setup
        log::debug({"Using Legacy Provide Buffers (Linux < 5.19)"});
    }
}
