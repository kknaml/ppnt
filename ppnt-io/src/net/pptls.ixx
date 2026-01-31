export module ppnt.net.tls;

import std;
import ppnt.traits;
import ppnt.io;
import ppnt.err;
import boringssl;
import ppnt.log;

export namespace ppnt::net {

    namespace detail {

        auto get_tls_spec_slot() -> int;
    }

    struct TlsExtensionConfig {
        uint16_t id;

        enum class Strategy {
            Auto,
            ForceData,
            Drop
        } strategy = Strategy::Auto;
        std::vector<uint8_t> data{};
    };

    struct ClientHelloSpec {
        std::vector<uint16_t> cipher_suites{};
        std::vector<uint8_t> compression_method = {0x00};
        std::vector<TlsExtensionConfig> extensions{};
    };
    
    class ClientHelloSpecFactory {
    public:
        constexpr ClientHelloSpecFactory() = default;
        virtual auto get_tls_spec() const -> ClientHelloSpec = 0;
        virtual auto is_global() const -> bool {
            return false;
        }
        virtual ~ClientHelloSpecFactory() = default;
    };
    
    namespace detail {
        template<typename Fn>
        class ClientHelloSpecFactoryImpl : public ClientHelloSpecFactory {
        private:
            Fn fn_;
            bool is_global_;
        public:
            explicit ClientHelloSpecFactoryImpl(Fn fn, bool is_global = false) : fn_(std::move(fn)), is_global_(is_global) {}
            
            auto get_tls_spec() const -> ClientHelloSpec override {
                return fn_();
            }

            auto is_global() const -> bool override {
                return is_global_;
            }
        };
    }

    /**
     * @brief Creates a ClientHelloSpecFactory instance.
     * * @note **Ownership Transfer**: The returned raw pointer is intended to be stored
     * in an SSL object via `SSL_set_ex_data`.
     * * @details
     * The lifecycle of this object is managed by the SSL ex_data free callback.
     * When `SSL_free()` is called:
     * - If `!is_global()`, this object will be automatically `delete`d.
     * - If `is_global()`, the deletion is skipped.
     * * @warning Do NOT manually delete the returned pointer if it has been attached to an SSL.
     */
    auto create_hello_spec_factory(auto &&fn, bool is_global = false) -> ClientHelloSpecFactory * {
        return new detail::ClientHelloSpecFactoryImpl(fn, is_global);
    }

    auto make_test_spec() -> ClientHelloSpec {
        auto result = ClientHelloSpec{};
        result.cipher_suites = {
        0x1a1a, // GREASE
        0x1301, // TLS_AES_128_GCM_SHA256
        0x1302, // TLS_AES_256_GCM_SHA384
        0x1303, // TLS_CHACHA20_POLY1305_SHA256
        0xc02b, // ECDHE-ECDSA-AES128-GCM-SHA256
        0xc02f, // ECDHE-RSA-AES128-GCM-SHA256
        };
        result.extensions = {
                {0x1a1a, TlsExtensionConfig::Strategy::ForceData, {0x00, 0x00}}, // GREASE (手动构造空数据)
            {0x0015, TlsExtensionConfig::Strategy::Auto}, // Padding (Auto: 让 BoringSSL 计算 padding)
            {0x0000, TlsExtensionConfig::Strategy::Auto}, // SNI (Server Name)
            {0x0017, TlsExtensionConfig::Strategy::Auto}, // Extended Master Secret
            {0x0023, TlsExtensionConfig::Strategy::Auto}, // Session Ticket
            {0x000d, TlsExtensionConfig::Strategy::Auto}, // Signature Algorithms
            {0x002b, TlsExtensionConfig::Strategy::Auto}, // Supported Versions
            {0x002d, TlsExtensionConfig::Strategy::Auto}, // PSK Key Exchange Modes
            {0x0033, TlsExtensionConfig::Strategy::Auto}, // Key Share (Key Share 数据极其复杂，必须用 Auto 让 BoringSSL 生成)
        };

        return result;
    }

    class ClientHelloRewriter {
    public:
        static auto rewrite(
            std::span<const uint8_t> original,
            const ClientHelloSpec &spec
        ) -> Result<std::vector<uint8_t>> {
            auto reader = io::BinaryReader(original);
            auto writer = io::BinartWriter{};

            auto handshake_type = reader.read_u8();
            auto handshake_len = reader.read_bytes(3);
            if (!handshake_type) return make_err_result(std::errc::bad_message, "cannot read handshake type");
            if (*handshake_type != 0x01) return make_err_result(std::errc::bad_message, "not a client hello"); // 0x01 = ClientHello
            if (!handshake_len) return make_err_result(std::errc::bad_message, "cannot read handshake len");

            auto legacy_ver = reader.read_u16();
            if (!legacy_ver) {
                return make_err_result(std::errc::bad_message, "cannot read legacy version");
            }
            auto random = reader.read_bytes(32);
            if (!random) {
                return make_err_result(std::errc::bad_message, "cannot read random");
            }
            auto session_id_len = reader.read_u8();
            if (!session_id_len) {
                return make_err_result(std::errc::bad_message, "cannot read session id len");
            }
            auto session_id = reader.read_bytes(*session_id_len);
            if (!session_id) {
                return make_err_result(std::errc::bad_message, "cannot read session id");
            }
            auto ciphers_len = reader.read_u16();
            if (!ciphers_len) {
                return make_err_result(std::errc::bad_message, "cannot read ciphers len");
            }
            if (!reader.read_bytes(*ciphers_len)) {
                return make_err_result(std::errc::bad_message, "cannot read ciphers");
            }
            auto comp_len = reader.read_u8();
            if (!comp_len) {
                return make_err_result(std::errc::bad_message, "cannot read compress len");
            }
            if (!reader.read_bytes(*comp_len)) {
                return make_err_result(std::errc::bad_message, "cannot read compress");
            }
            // std::map<uint16_t, std::span<const uint8_t>> parsed_extensions{};
            std::vector<std::pair<uint16_t, std::span<const uint8_t>>> parsed_extensions;
            auto ext_block_len = reader.read_u16();
            if (ext_block_len) {
                auto ext_block_data = reader.read_bytes(*ext_block_len);
                if (!ext_block_data) {
                    return make_err_result(std::errc::bad_message, "cannot read ext_block_data");
                }
                auto ex_reader = io::BinaryReader{*ext_block_data};
                while (ex_reader.remaining()) {
                    auto type = ex_reader.read_u16();
                    auto len = ex_reader.read_u16();
                    if (!type || !len) {
                        return make_err_result(std::errc::bad_message, "cannot read type or len");
                    }
                    auto data = ex_reader.read_bytes(*len);
                    if (!data) {
                        return make_err_result(std::errc::bad_message, "cannot read data");
                    }
                    parsed_extensions.emplace_back(*type, *data);
                }
            }

            writer.write_u8(0x01);
            auto len_pos_idx = writer.size();
            writer.write_u8(0);
            writer.write_u8(0);
            writer.write_u8(0);

            auto body_start_idx = writer.size();

            writer.write_u16(*legacy_ver);
            writer.write_bytes(*random);
            writer.write_u8(*session_id_len);
            writer.write_bytes(*session_id);

            writer.write_u16_length_prefixed([&] {
                for (auto cipher : spec.cipher_suites) {
                    writer.write_u16(cipher);
                }
            });

            writer.write_u8(static_cast<uint8_t>(spec.compression_method.size()));
            writer.write_bytes(spec.compression_method);

            writer.write_u16_length_prefixed([&] {
                for (const auto &ext_cfg : spec.extensions) {
                    if (ext_cfg.strategy == TlsExtensionConfig::Strategy::Drop) {
                        continue;
                    }
                    std::span<const uint8_t> payload{};
                    auto should_write = false;
                    if (ext_cfg.strategy == TlsExtensionConfig::Strategy::ForceData) {
                        payload = ext_cfg.data;
                        should_write = true;
                    } else {
                        auto it = std::ranges::find_if(parsed_extensions,
                       [&](const auto& p) {
                           return p.first == ext_cfg.id;
                       });

                        if (it != parsed_extensions.end()) {
                            payload = it->second;
                            should_write = true;
                        }
                  }

                   if (should_write) {
                       writer.write_u16(ext_cfg.id);
                       writer.write_u16(static_cast<uint16_t>(payload.size()));
                       writer.write_bytes(payload);
                   }
                }
            });

            auto body_end_idx = writer.size();
            size_t body_len = body_end_idx - body_start_idx;
            writer.buffer()[len_pos_idx]     = static_cast<uint8_t>(body_len >> 16);
            writer.buffer()[len_pos_idx + 1] = static_cast<uint8_t>(body_len >> 8);
            writer.buffer()[len_pos_idx + 2] = static_cast<uint8_t>(body_len);

            return std::move(writer).leak();
        }
    };

    auto my_client_hello_interceptor(boringssl::SSL *ssl, uint8_t **data_ptr, size_t *len_ptr) -> int {
        log::info({"aaaaa my_client_hello_interceptor"});
        auto *spec_factory = static_cast<ClientHelloSpecFactory *>(boringssl::SSL_get_ex_data(ssl, detail::get_tls_spec_slot()));
        if (!spec_factory) {
            return 1;
        }
        auto spec = spec_factory->get_tls_spec();
        auto original = std::span{*data_ptr, *len_ptr};
        auto new_hello_vec = ClientHelloRewriter::rewrite(original, spec);
        if (!new_hello_vec) {
            log::error({"error in tls rewrite: {}"}, new_hello_vec.error());
            return 1;
        }
        if (new_hello_vec->empty()) {
            log::error({"tls rewrite empty"});
            return 1;
        }
        auto *new_mem = static_cast<uint8_t *>(boringssl::OPENSSL_malloc(new_hello_vec->size()));
        if (!new_mem) {
            log::error({"error in tls malloc"});
            return 0;
        }
        std::memcpy(new_mem, new_hello_vec->data(), new_hello_vec->size());
        *data_ptr = new_mem;
        *len_ptr = new_hello_vec->size();
        return 1;
    }
}
