module ppnt.net.ssl;

import ppnt.net.tls;

namespace ppnt::net {

    auto SslFree::operator()(boringssl::SSL *ptr) -> void {
        if (ptr != nullptr) {
            // auto *spec_factory = static_cast<ClientHelloSpecFactory *>(boringssl::SSL_get_ex_data(ptr, detail::get_tls_spec_slot()));
            // if (spec_factory != nullptr) {
            //     if (!spec_factory->is_global()) {
            //         delete spec_factory;
            //     }
            // }
            boringssl::SSL_free(ptr);
        }
    }
}