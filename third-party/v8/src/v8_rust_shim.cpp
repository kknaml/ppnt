#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#define RUST_FUNC extern "C" __attribute__((visibility("default"), used))

#define RUST_VAR __attribute__((visibility("default"), used))

extern "C" {

    RUST_FUNC void *__rust_alloc(size_t size, size_t align) { return malloc(size); }
    RUST_FUNC void __rust_dealloc(void *ptr, size_t size, size_t align) { free(ptr); }
    RUST_FUNC void *__rust_realloc(void *ptr, size_t old_size, size_t align, size_t new_size) { return realloc(ptr, new_size); }
    RUST_FUNC void *__rust_alloc_zeroed(size_t size, size_t align) { return calloc(1, size); }

    RUST_VAR uint8_t __rust_no_alloc_shim_is_unstable = 0;
    RUST_VAR uint8_t __rust_no_alloc_shim_is_unstable_v2 = 0;

    RUST_FUNC void *_RNvCs7JQRz1lIFu6_7___rustc12___rust_alloc(size_t size, size_t align) {
        return malloc(size);
    }

    RUST_FUNC void *_RNvCs7JQRz1lIFu6_7___rustc14___rust_realloc(void *ptr, size_t old_size, size_t align, size_t new_size) {
        return realloc(ptr, new_size);
    }

    RUST_FUNC void _RNvCs7JQRz1lIFu6_7___rustc14___rust_dealloc(void *ptr, size_t size, size_t align) {
        free(ptr);
    }

    RUST_FUNC void *_RNvCs7JQRz1lIFu6_7___rustc19___rust_alloc_zeroed(size_t size, size_t align) {
        return calloc(1, size);
    }

    RUST_VAR uint8_t _RNvCs7JQRz1lIFu6_7___rustc35___rust_no_alloc_shim_is_unstable_v2 = 0;
}
