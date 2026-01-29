
module ppnt.libc;

extern "C" int *__errno_location (void) throw();

namespace ppnt::libc {
    auto error_no() -> int {
        return *__errno_location();
    }
}
