module;

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

export module ppnt.libc:socket;



export namespace ppnt::libc {
    using ::socklen_t;
    using ::sockaddr;
    using ::sockaddr_in;
    using ::sockaddr_in6;
    using ::msghdr;
    using ::timespec;
    using ::ssize_t;
    using ::sockaddr_storage;
    using ::sa_family_t;
    using ::addrinfo;
    using ::freeaddrinfo;

    using ::inet_pton;
    using ::ntohs;
    using ::ntohl;
    using ::htons;
    using ::inet_ntop;
    using ::getaddrinfo;

    using ::socket;
    using ::socketpair;
    using ::bind;
    using ::listen;
    using ::accept;
    using ::accept4;
    using ::connect;
    using ::send;
    using ::recv;
    using ::getsockname;
    using ::getpeername;
    using ::sendto;
    using ::recvfrom;
    using ::sendmsg;
    using ::recvmsg;
    using ::sendmmsg;
    using ::recvmmsg;
    using ::getsockopt;
    using ::setsockopt;
    using ::shutdown;
    using ::sockatmark;
    using ::isfdtype;

    constexpr int inet6_addrstrlen = INET6_ADDRSTRLEN;
    constexpr int inet_addrstrlen = INET_ADDRSTRLEN;

    constexpr int af_unspec  = AF_UNSPEC;
    constexpr int af_local   = AF_LOCAL;   // = AF_UNIX
    constexpr int af_unix    = AF_UNIX;
    constexpr int af_inet    = AF_INET;
    constexpr int af_inet6   = AF_INET6;
    constexpr int af_netlink = AF_NETLINK;

    constexpr int sock_stream    = SOCK_STREAM;
    constexpr int sock_dgram     = SOCK_DGRAM;
    constexpr int sock_seqpacket = SOCK_SEQPACKET;
    constexpr int sock_raw       = SOCK_RAW;

    constexpr int sol_socket = SOL_SOCKET;
    constexpr int sol_tcp    = IPPROTO_TCP;
    constexpr int sol_udp    = IPPROTO_UDP;

    constexpr int so_reuseaddr = SO_REUSEADDR;
    constexpr int so_reuseport = SO_REUSEPORT;
    constexpr int so_error     = SO_ERROR;
    constexpr int so_rcvbuf    = SO_RCVBUF;
    constexpr int so_sndbuf    = SO_SNDBUF;
    constexpr int so_keepalive = SO_KEEPALIVE;


    constexpr int tcp_nodelay  = TCP_NODELAY;
    constexpr int tcp_cork     = TCP_CORK;
    constexpr int tcp_keepidle = TCP_KEEPIDLE;
    constexpr int tcp_keepintvl = TCP_KEEPINTVL;
    constexpr int tcp_keepcnt  = TCP_KEEPCNT;

    constexpr int sock_nonblock = SOCK_NONBLOCK;
    constexpr int sock_cloexec  = SOCK_CLOEXEC;

    constexpr int msg_dontwait = MSG_DONTWAIT;
    constexpr int msg_nosignal = MSG_NOSIGNAL;
    constexpr int msg_peek     = MSG_PEEK;

    constexpr int shut_rd   = SHUT_RD;
    constexpr int shut_wr   = SHUT_WR;
    constexpr int shut_rdwr = SHUT_RDWR;


}

//
// export extern "C" {
//
//
//     extern auto bind(int fd, const sockaddr *addr, socklen_t len) -> int;
//     extern auto getsockname(int fd, sockaddr *addr, socklen_t *len) -> int;
//     extern auto connect(int fd, const sockaddr *addr, socklen_t len) -> int;
//     extern auto getpeername(int fd, sockaddr *addr, socklen_t *len) -> int;
//     extern auto send(int fd, const void *buf, size_t len, int flags) -> long;
//     extern auto recv(int fd, void *buf, size_t len, int flags) -> long;
//     extern auto sendto(int fd, const void *buf, size_t len, int flags, const sockaddr *addr, socklen_t addrlen) -> long;
//     extern auto recvfrom(int fd, void *buf, size_t n, int flag, sockaddr *addr, socklen_t *addrlen) -> long;
//     extern auto sendmsg(int fd, const msghdr *message, int flags) -> long;
//     extern auto sendmmsg(int fd, const msghdr *message, unsigned int vlen, int flags) -> long;
//     extern auto recvmsg(int fd, msghdr *message, int flags) -> long;
//     extern auto recvmmsg(int fd, msghdr *message, unsigned int vlen, int flags, timespec *tmo) -> long;
//     extern auto getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen) -> int;
//     extern auto setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) -> int;
//     extern auto listen(int fd, int n) -> int;
//     extern auto accept(int fd, sockaddr *addr, socklen_t *addrlen) -> int;
//     extern auto accept4(int fd, sockaddr *addr, socklen_t *addrlen, int flags) -> int;
//     extern auto shutdown(int fd, int how) -> int;
//     extern auto sockatmark(int fd) -> int;
//     extern auto isfdtype(int fd, int fdtype) -> int;
//
// }
