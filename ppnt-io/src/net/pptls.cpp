module ppnt.net.tls;

namespace ppnt::net {

    namespace detail {

        auto get_tls_spec_slot() -> int {
            static int g_spec_idx = boringssl::SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
            return g_spec_idx;
        }
    }

}