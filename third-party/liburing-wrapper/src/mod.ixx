
module;

#include <liburing.h>

export module liburing;

export namespace liburing {
    // Types
    using ::io_uring;
    using ::io_uring_sqe;
    using ::io_uring_cqe;
    using ::io_uring_params;
    using ::io_uring_probe;
    using ::__kernel_timespec;

    // Core functions
    using ::io_uring_queue_init;
    using ::io_uring_queue_init_params;
    using ::io_uring_queue_exit;
    using ::io_uring_submit;
    using ::io_uring_submit_and_wait;
    using ::io_uring_get_sqe;
    using ::io_uring_wait_cqe;
    using ::io_uring_peek_cqe;
    using ::io_uring_wait_cqes;
    using ::io_uring_peek_batch_cqe;
    using ::io_uring_cqe_seen;

    // Data helpers
    using ::io_uring_sqe_set_data;
    using ::io_uring_sqe_set_data64;
    using ::io_uring_cqe_get_data;
    using ::io_uring_cqe_get_data64;

    // Prep functions
    using ::io_uring_prep_read;
    using ::io_uring_prep_write;
    using ::io_uring_prep_readv;
    using ::io_uring_prep_writev;
    using ::io_uring_prep_recvmsg;
    using ::io_uring_prep_sendmsg;
    using ::io_uring_prep_poll_add;
    using ::io_uring_prep_poll_remove;
    using ::io_uring_prep_poll_update;
    using ::io_uring_prep_accept;
    using ::io_uring_prep_connect;
    using ::io_uring_prep_close;
    using ::io_uring_prep_timeout;
    using ::io_uring_prep_timeout_remove;
    using ::io_uring_prep_nop;
    using ::io_uring_prep_cancel;

    // CQE Iteration
    using ::io_uring_cq_advance;
    using ::io_uring_cqe_iter;
    using ::io_uring_cqe_iter_init;
    using ::io_uring_cqe_iter_next;
}
