export module ppnt.net.accept;

import std;
import ppnt.common;
import ppnt.net.addr;
import ppnt.net.tcp_stream;
import ppnt.io.task;
import ppnt.io.driver;
import ppnt.io.runtime;
import liburing;

export namespace ppnt::net {

    class AcceptorTask;

    namespace detail {

        struct AcceptFinalAwaiter {
            constexpr auto await_ready() const noexcept -> bool { return false; }

            template<typename Promise>
            auto await_suspend(std::coroutine_handle<Promise> handle) const noexcept {
                auto &promise = handle.promise();
                if (promise.exception) {
                    try {
                        std::rethrow_exception(promise.exception);
                    } catch (std::exception &e) {
                        log::error({"Acceptor Error: {}"}, e.what());
                    } catch (...) {
                        log::error({"Acceptor Unknown Error!"});
                    }
                }
            }

            auto await_resume() const noexcept -> void {

            }
        };

        struct AcceptorPromise {
            io::Operation operation{};
            std::exception_ptr exception{nullptr};

            constexpr auto initial_suspend() const noexcept -> std::suspend_never {
                return {};
            }

            constexpr auto final_suspend() const noexcept -> AcceptFinalAwaiter {
                return {};
            }

            auto unhandled_exception() noexcept -> void {
                this->exception = std::current_exception();
            }


            auto get_return_object() -> AcceptorTask;

            constexpr auto return_void() noexcept -> void {}
        };

        struct AcceptDataAwaiter {
            AcceptorPromise *data{nullptr};
            constexpr auto await_ready() const noexcept -> bool {
                return false;
            }

            auto await_suspend(std::coroutine_handle<AcceptorPromise> handle) -> bool {
                data = &handle.promise();
                return false;
            }

            auto await_resume() noexcept -> AcceptorPromise * {
                return data;
            }
        };

        auto init_accept_socket(SocketAddress &addr) -> int;

        auto prep_accept(int fd, io::Operation *operation) -> void;

        struct AcceptTaskAwaiter {
            constexpr auto await_ready() const noexcept -> bool { return false; }

            auto await_suspend(std::coroutine_handle<> handle) noexcept {}

            auto await_resume() noexcept {}
        };
    }


    class [[nodiscard]] AcceptorTask {
    public:
        using promise_type = detail::AcceptorPromise;
    private:
        std::coroutine_handle<promise_type> handle;

    public:
        explicit AcceptorTask(std::coroutine_handle<promise_type> handle) noexcept : handle(handle) {}


        auto operator co_await() -> detail::AcceptTaskAwaiter {
            return {};
        }
    };



    auto run_accept_loop(auto &&handler, SocketAddress addr) -> AcceptorTask {
        auto *promise = co_await detail::AcceptDataAwaiter{};
        auto handle = std::coroutine_handle<AcceptorTask::promise_type>::from_promise(*promise);
        promise->operation.handle = handle;
        auto socket_fd = detail::init_accept_socket(addr);
        if (socket_fd < 0) {
            // TODO
            log::error({"Acceptor Error: {}"}, socket_fd);
            std::terminate();
        }
        detail::prep_accept(socket_fd, &promise->operation);
        log::info({"Server listen on {}"}, addr);

        while (true) {
            // multishot accept, just await this to get results
            co_await std::suspend_always{};
            auto cqe_flags = promise->operation.data.cqe_flags;
            if (!(cqe_flags & liburing::ioring_cqe_f_more)) {
                // TODO
                log::error({"cqe f more failed"});
                std::terminate();
            }
            auto fd = promise->operation.data.result;
            if (fd >= 0) {
                io::spawn(handler(TcpStream(fd))).take_handle();
            } else {
                // TODO
                log::error({"Acceptor Error: {}"}, fd);
                std::terminate();
            }
            promise->operation.reset();
            promise->operation.handle = handle;
        }
    }
}