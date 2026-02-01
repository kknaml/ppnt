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

    auto is_h2_negotiated(boringssl::SSL *ssl) -> bool {
        const unsigned char *data{nullptr};
        unsigned int len = 0;
        boringssl::SSL_get0_alpn_selected(ssl, &data, &len);

        if (len == 2 && std::memcmp(data, "h2", 2) == 0) {
            return true;
        }
        return false;
    }
}