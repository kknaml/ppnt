module ppnt.net.addr;

// gcc internal error
// #include <netdb.h>
// #include <cerrno>

extern "C" int *__errno_location (void) throw();

namespace ppnt::net {

    static constexpr inline int EAI_SYSTEM = -11;
    static constexpr inline int EAI_NONAME = -2;
    static constexpr inline int EAI_SERVICE = -8;
    static constexpr inline int EAI_NODATA = -5;
    static constexpr inline int EAI_AGAIN = -3;
    static constexpr inline int EAI_MEMORY = -10;


    namespace detail {
        auto map_getaddrinfo_err(int err, std::string msg) -> Error {
            if (err ==  EAI_SYSTEM) {
                return Error(std::error_code(*__errno_location(), std::generic_category()));
            }
            std::errc code;
            switch (err) {
                case EAI_NONAME:
                case EAI_SERVICE:
                case EAI_NODATA:
                    code = std::errc::address_not_available;
                    break;
                case EAI_AGAIN:
                    code = std::errc::resource_unavailable_try_again;
                    break;
                case EAI_MEMORY:
                    code = std::errc::not_enough_memory;
                    break;
                default:
                    code = std::errc::invalid_argument;
                    break;
            }
            return Error(std::make_error_code(code), std::move(msg));

        }
    }
}
