module ppnt.net.tls;

namespace ppnt::net {

    namespace {
        auto client_hello_spec_free_callback(
            void *parent,
            void *ptr,
            boringssl::CRYPTO_EX_DATA *ad,
            int idx,
            long argl,
            void *argp
        ) -> void {
            auto *factory = static_cast<ClientHelloSpecFactory *>(ptr);
            if (factory != nullptr) {
                if (!factory->is_global()) {
                    log::warn({"delete spec factory"});
                    delete factory;
                }
            }
        }
    }

    namespace detail {

        auto get_tls_spec_slot() -> int {
            static int g_spec_idx = boringssl::SSL_get_ex_new_index(
                0,
                nullptr,
                nullptr,
                nullptr,
                client_hello_spec_free_callback
            );
            return g_spec_idx;
        }
    }

}