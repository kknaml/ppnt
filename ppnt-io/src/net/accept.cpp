module ppnt.net.accept;

import ppnt.libc;
import liburing;

namespace ppnt::net {

    namespace detail {
        auto AcceptorPromise::get_return_object() -> AcceptorTask {
            return AcceptorTask{std::coroutine_handle<detail::AcceptorPromise>::from_promise(*this)};
        }

        auto init_accept_socket(SocketAddress &addr) -> int {
            auto socket = libc::socket(libc::af_inet, libc::sock_stream | libc::sock_cloexec, 0);
            if (socket < 0) {
                log::error({"Socket creation failed: {}"}, socket);
                return socket;
            }
            int opt = 1;
            libc::setsockopt(socket, libc::sol_socket, libc::so_reuseaddr, &opt, sizeof(opt));

            auto res = libc::bind(socket, addr.sockaddr_ptr(), addr.socklen());
            if (res < 0) {
                log::error({"Socket bind failed: {}"}, res);
                libc::close(socket);
                return res;
            }
            res = libc::listen(socket, 4096);
            if (res < 0) {
                log::error({"Listen failed: {}"}, res);
                libc::close(socket);
                return res;
            }
            return socket;
        }

        auto prep_accept(int fd, io::Operation *operation) -> void {
            auto &ring = io::Ring::current();

            ring.submit(*operation, [=](liburing::io_uring_sqe *sqe) {
                liburing::io_uring_prep_multishot_accept(sqe, fd, nullptr, nullptr, 0);
            });
        }


    }

}
