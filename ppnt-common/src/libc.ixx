
export module ppnt.libc;

export import :io;
export import :socket;
export import :fcntl;
export import :unistd;
export import :event;
export import :err;

export namespace ppnt::libc {

    auto error_no() -> int;
}