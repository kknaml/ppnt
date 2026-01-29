export module ppnt.io.runtime;

import std;
import ppnt.io.task;
import ppnt.io.driver;
import ppnt.traits;

namespace ppnt::io {

    export template<typename T>
    auto block_on(Task<T> &&task) -> Regularized<T> {
        Ring ring;
        Ring::set_ring(&ring);

        auto handle = task.take_handle();
        
        // We need to keep the ring spinning until the task is done.
        // But handle.resume() only starts it. Who wakes up block_on loop?
        // Simple approach: The task promise sets a flag or we check handle.done().
        // Since we are the root, and we are single threaded, we can just run ring until handle is done.
        
        if (!handle) std::terminate();

        handle.resume();

        while (!handle.done()) {
            ring.run_one();
        }
        
        // Get value before destroying
        auto result = std::move(handle.promise()).get_value();
        handle.destroy();
        
        Ring::set_ring(nullptr);
        return result;
    }

    export template<typename T>
    auto spawn(Task<T> &&task) -> JoinHandle<T> {
        auto handle = task.take_handle();
        if (handle) {
            handle.resume();
        }
        return JoinHandle<T>{handle};
    }

    export template<typename Fn>
        requires std::is_invocable_v<Fn>
    auto spawn(Fn &&fn) {
        return spawn(fn());
    }

    export template<typename Fn>
        requires std::is_invocable_v<Fn>
    auto block_on(Fn &&fn) {
        return block_on(fn());
    }

}
