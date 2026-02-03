module;

#include <sys/eventfd.h>

export module ppnt.libc:event;

export namespace ppnt::libc {

    using ::eventfd;
    // using ::EFD_SEMAPHORE;
    // using ::EFD_CLOEXEC;
    // using ::EFD_NONBLOCK;

    constexpr int efd_semaphore = EFD_SEMAPHORE;
    constexpr int efd_cloexec = EFD_CLOEXEC;
    constexpr int efd_nonblock = EFD_NONBLOCK;
}
