module ppnt.net.ssl;

import ppnt.net.tls;

namespace ppnt::net {

    auto SslFree::operator()(boringssl::SSL *ptr) -> void {
        if (ptr != nullptr) {
            if (auto *spec = static_cast<ClientHelloSpec *>(boringssl::SSL_get_ex_data(ptr, detail::get_tls_spec_slot()));
                spec != nullptr) {
                boringssl::SSL_set_ex_data(ptr, detail::get_tls_spec_slot(), nullptr);
                delete spec;
            }
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
