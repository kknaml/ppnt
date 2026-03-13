module;

#include <cerrno>

export module ppnt.libc:err;

export namespace ppnt::libc {

    constexpr int e_again = EAGAIN;
    constexpr int e_wouldblock = EWOULDBLOCK;
    constexpr int e_dead_lk = EDEADLK;
    constexpr int e_name_too_long = ENAMETOOLONG;
    constexpr int e_no_lck = ENOLCK;
    constexpr int e_no_sys = ENOSYS;
    constexpr int e_not_empty = ENOTEMPTY;
    constexpr int e_loop = ELOOP;
    constexpr int e_time = ETIME;
    constexpr int e_canceled = ECANCELED;
}
