
export module ppnt.libc;

export import :io;
export import :socket;
export import :fcntl;
export import :unistd;

export namespace ppnt::libc {

    auto error_no() -> int;
}