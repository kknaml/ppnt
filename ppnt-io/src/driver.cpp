module ppnt.io.driver;

import ppnt.log;

namespace ppnt::io {

    thread_local Ring *current_ring = nullptr;

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

}
