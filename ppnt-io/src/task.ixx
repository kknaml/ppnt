export module ppnt.io.task;

import std;
import ppnt.common;

namespace ppnt::io {

    export template<typename T = void>
    class Task;

    export template<typename T>
    using TaskResult = Task<Result<T>>;

    export template<typename T>
    class JoinHandle;

    namespace detail {
        struct TaskFinalAwaiter {
            constexpr auto await_ready() const noexcept -> bool { return false; }

            template<typename Promise>
            auto await_suspend(std::coroutine_handle<Promise> current) const noexcept -> std::coroutine_handle<> {
                auto &promise = current.promise();
                if (promise.exception) {
                    try {
                        std::rethrow_exception(promise.exception);
                    } catch (std::exception &e) {
                        log::error({"Task exception: {}"}, e.what());
                    } catch (...) {
                        log::error({"Task unkown exception"});
                    }
                }
                if (promise.parent) {
                    return promise.parent;
                }
                return std::noop_coroutine();
            }

            constexpr auto await_resume() const noexcept -> void {}
        };

        template<typename Promise>
        struct BaseTaskAwaiter {
            std::coroutine_handle<Promise> current{nullptr};

            explicit BaseTaskAwaiter(std::coroutine_handle<Promise> p) noexcept : current {p} {}

            auto await_ready() const noexcept -> bool {
                return  !current || current.done();
            }

            template<typename Promise2>
            auto await_suspend(std::coroutine_handle<Promise2> parent) -> std::coroutine_handle<> {
                current.promise().parent = parent;
                return current;
            }

            ~BaseTaskAwaiter() {
                if (current) {
                    current.destroy();
                }
            }
        };

        template<typename T>
        struct BaseTaskPromise {
            using ValueType = Regularized<T>;
            std::optional<ValueType> value{std::nullopt};
            std::coroutine_handle<> parent{nullptr};
            std::exception_ptr exception{nullptr};

            constexpr auto initial_suspend() const noexcept -> std::suspend_always {
                return {};
            }

            constexpr auto final_suspend() const noexcept -> TaskFinalAwaiter {
                return {};
            }

            auto unhandled_exception() noexcept -> void {
                this->exception = std::current_exception();
            }

            auto get_value() & -> ValueType & {
                this->check_error();
                return this->value.value();
            }

            auto get_value() && -> ValueType {
                this->check_error();
                return std::move(this->value.value());
            }

            auto has_value() const noexcept -> bool {
                return this->value != std::nullopt;
            }

            auto is_done() const noexcept -> bool {
                return this->has_value() || this->exception != nullptr;
            }

            auto check_error() const {
                if (this->exception) {
                    std::rethrow_exception(this->exception);
                }
            }

            template<typename U>
            auto await_transform(Task<U> &&task) {
                struct Awaiter final : BaseTaskAwaiter<typename Task<U>::promise_type> {
                    using BaseTaskAwaiter<typename Task<U>::promise_type>::BaseTaskAwaiter;
                    auto await_resume() -> decltype(auto) {
                        return std::move(this->current.promise()).get_value();
                    }
                };
                return Awaiter{task.take_handle()};
            }

            template<typename Awaiter>
         auto await_transform(Awaiter &&awaiter) -> decltype(auto) {
                return std::forward<Awaiter>(awaiter);
            }

        };

        template<typename T>
        struct TaskPromise final : BaseTaskPromise<T> {
            auto get_return_object() noexcept -> Task<T>;
            auto return_value(T t) -> void {
                this->value = std::move(t);
            }
        };

        template<>
        struct TaskPromise<void> final : BaseTaskPromise<void> {
            auto get_return_object() noexcept -> Task<>;
            auto return_void() noexcept -> void {
                this->value = std::monostate{};
            }
        };

        template<typename T>
        struct JoinHandleAwaiter final : BaseTaskAwaiter<typename Task<T>::promise_type> {
            using BaseTaskAwaiter<typename Task<T>::promise_type>::BaseTaskAwaiter;

            auto await_resume() -> Regularized<T> {
                return std::move(this->current.promise()).get_value();
            }
        };

    }

    template<typename T>
    [[nodiscard]]
    class Task final : NonCopy {
    public:
        using promise_type = detail::TaskPromise<T>;
    private:
        std::coroutine_handle<promise_type> handle;
    public:
        explicit Task(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}

        Task(Task &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

        auto operator=(Task &&other) noexcept -> Task & {
            if (this != &other) {
                this->handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        auto get_handle() const noexcept -> std::coroutine_handle<promise_type> {
            return handle;
        }

        auto take_handle() noexcept -> std::coroutine_handle<promise_type> {
            return std::exchange(handle, nullptr);
        }

        ~Task() {
            auto h = std::exchange(handle, nullptr);
            if (h) {
                h.destroy();
            }
        }
    };

    export template<typename T>
    [[nodiscard]]
    class JoinHandle final : NonCopy {
        using promise_type = detail::TaskPromise<T>;
        std::coroutine_handle<promise_type> handle;

    public:
        explicit JoinHandle(std::coroutine_handle<promise_type> h) noexcept : handle(h) {}

        JoinHandle(JoinHandle &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

        auto operator=(JoinHandle &&other) noexcept -> JoinHandle & {
            if (this != &other) {
                if (handle) handle.destroy();
                handle = std::exchange(other.handle, nullptr);
            }
            return *this;
        }

        ~JoinHandle() {
            if (handle) handle.destroy();
        }

        auto operator co_await() noexcept {
            return detail::JoinHandleAwaiter<T>{std::exchange(handle, nullptr)};
        }
    };

    namespace detail {
        template<typename T>
        auto TaskPromise<T>::get_return_object() noexcept -> Task<T> {
            return Task<T>(std::coroutine_handle<TaskPromise>::from_promise(*this));
        }


        auto TaskPromise<void>::get_return_object() noexcept -> Task<void> {
            return Task<void>(std::coroutine_handle<TaskPromise>::from_promise(*this));
        }
    }

}
