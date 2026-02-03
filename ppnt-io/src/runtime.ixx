export module ppnt.io.runtime;

import std;
import ppnt.io.task;
import ppnt.io.driver;
import ppnt.traits;
import ppnt.err;

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

    namespace detail {
        template<typename T, typename F, typename Pool>
        struct RunInPoolAwaiter {
            F func_;
            Ring *ring_;
            Pool *pool_;
            std::optional<Regularized<T>> res_{};
            std::exception_ptr err_{};

            RunInPoolAwaiter(Pool *pool, F &&f) : func_(std::move(f)), ring_(&Ring::current()), pool_(pool) {}

            auto await_ready() const noexcept -> bool {
                return false;
            }

            auto await_suspend(std::coroutine_handle<> h) noexcept -> void {
                pool_->enqueue_detach([this, h] () mutable {
                    try {
                        if constexpr (std::is_void_v<T>) {
                            func_();
                            res_ = Unit{};
                        } else {
                            res_ = func_();
                        }
                    } catch (...) {
                        err_ = std::current_exception();
                    }
                    ring_->post(h);
                });
            }

            auto await_resume() noexcept -> Result<T> {
                if (err_) {
                    try {
                        std::rethrow_exception(err_);
                    } catch (std::exception &e) {
                        return make_err_result(std::errc::no_message, e.what());
                    } catch (...) {
                        return make_err_result(std::errc::no_message, "run_in_pool err");
                    }
                }
                return std::move(*res_);
            }
        };
    }

    template<typename T, typename Pool>
    auto run_in_pool(Pool *pool, Func<T> auto &&fn) -> detail::RunInPoolAwaiter<T, decltype(fn), Pool> {
        return detail::RunInPoolAwaiter<T, decltype(fn), Pool>{pool, fn};
    }
}
