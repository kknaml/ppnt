module;

#include <sys/mman.h>

export module ppnt.libc:mman;

export namespace ppnt::libc {

    using ::mmap;
    using ::mmap64;
    using ::munmap;
    using ::mprotect;
    using ::msync;
    using ::mlock;
    using ::munlock;
    using ::mlockall;
    using ::munlockall;
    using ::madvise;
    using ::mincore;
    using ::shm_open;
    using ::shm_unlink;

    constexpr int prot_read = PROT_READ;
    constexpr int prot_write = PROT_WRITE;
    constexpr int prot_exec = PROT_EXEC;
    constexpr int prot_none = PROT_NONE;
    constexpr int prot_growsdown = PROT_GROWSDOWN;
    constexpr int prot_growsup = PROT_GROWSUP;

    // Sharing types
    constexpr int map_shared = MAP_SHARED;
    constexpr int map_private = MAP_PRIVATE;
    constexpr int map_shared_validate = MAP_SHARED_VALIDATE;
    constexpr int map_type = MAP_TYPE;
    constexpr int map_fixed = MAP_FIXED;
    constexpr int map_file = MAP_FILE;
    constexpr int map_anonymous = MAP_ANONYMOUS;
    constexpr int map_anon = MAP_ANON;
    constexpr int map_huge_shift = MAP_HUGE_SHIFT;
    constexpr int map_huge_mask = MAP_HUGE_MASK;
    constexpr int ms_async = MS_ASYNC;
    constexpr int ms_sync = MS_SYNC;
    constexpr int ms_invalidate = MS_INVALIDATE;

    constexpr int madv_normal = MADV_NORMAL;
    constexpr int madv_random = MADV_RANDOM;
    constexpr int madv_sequential = MADV_SEQUENTIAL;
    constexpr int madv_willneed = MADV_WILLNEED;
    constexpr int madv_dontneed = MADV_DONTNEED;
    constexpr int madv_free = MADV_FREE;
    constexpr int madv_remove = MADV_REMOVE;
    constexpr int madv_dontfork = MADV_DONTFORK;
    constexpr int madv_dofork = MADV_DOFORK;
    constexpr int madv_mergeable = MADV_MERGEABLE;
    constexpr int madv_unmergeable = MADV_UNMERGEABLE;
    constexpr int madv_hugepage = MADV_HUGEPAGE;
    constexpr int madv_nohugepage = MADV_NOHUGEPAGE;
    constexpr int madv_dontdump = MADV_DONTDUMP;
    constexpr int madv_dodump = MADV_DODUMP;
    constexpr int madv_wipeonfork = MADV_WIPEONFORK;
    constexpr int madv_keeponfork = MADV_KEEPONFORK;
    constexpr int madv_cold = MADV_COLD;
    constexpr int madv_pageout = MADV_PAGEOUT;
    // constexpr int madv_populate_read = MADV_POPULATE_READ;
    // constexpr int madv_populate_write = MADV_POPULATE_WRITE;
    // constexpr int madv_dontneed_locked = MADV_DONTNEED_LOCKED;
    constexpr int madv_hwpoison = MADV_HWPOISON;

    constexpr int posix_madv_normal = POSIX_MADV_NORMAL;
    constexpr int posix_madv_random = POSIX_MADV_RANDOM;
    constexpr int posix_madv_sequential = POSIX_MADV_SEQUENTIAL;
    constexpr int posix_madv_willneed = POSIX_MADV_WILLNEED;
    constexpr int posix_madv_dontneed = POSIX_MADV_DONTNEED;

    constexpr int mcl_current = MCL_CURRENT;
    constexpr int mcl_future = MCL_FUTURE;
    constexpr int mcl_onfault = MCL_ONFAULT;


}