module;

#include <fcntl.h>

export module ppnt.libc:fcntl;

export namespace ppnt::libc {
    using ::fcntl;
    using ::fcntl64;
    using ::open;
    using ::open64;
    using ::openat;
    using ::openat64;
    using ::creat;
    using ::creat64;
    using ::lockf;
    using ::lockf64;
    using ::posix_fadvise;
    using ::posix_fadvise64;
    using ::posix_fallocate;
    using ::posix_fallocate64;
}