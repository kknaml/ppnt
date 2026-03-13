module ppnt.net.tls;

namespace ppnt::net {

    namespace {
    }

    namespace detail {

        auto get_tls_spec_slot() -> int {
            static int g_spec_idx = boringssl::SSL_get_ex_new_index(
                0,
                nullptr,
                nullptr,
                nullptr,
                nullptr
            );
            return g_spec_idx;
        }
    }

    auto ClientHelloRewriter::rewrite(
        std::span<const uint8_t> original,
        const ClientHelloSpec &spec
    ) -> Result<std::vector<uint8_t> > {
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
        if (!ext_block_len) {
            return make_err_result(std::errc::bad_message, "cannot read extension block len");
        }

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
            std::vector<uint16_t> handled_ext_ids{};
            handled_ext_ids.reserve(spec.extensions.size());

            for (const auto &ext_cfg : spec.extensions) {
                handled_ext_ids.push_back(ext_cfg.id);
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

            for (const auto &[type, data] : parsed_extensions) {
                auto already_handled = std::ranges::find(handled_ext_ids, type) != handled_ext_ids.end();
                if (already_handled) {
                    continue;
                }
                writer.write_u16(type);
                writer.write_u16(static_cast<uint16_t>(data.size()));
                writer.write_bytes(data);
            }
        });

        auto body_end_idx = writer.size();
        size_t body_len = body_end_idx - body_start_idx;
        writer.buffer()[len_pos_idx]     = static_cast<uint8_t>(body_len >> 16);
        writer.buffer()[len_pos_idx + 1] = static_cast<uint8_t>(body_len >> 8);
        writer.buffer()[len_pos_idx + 2] = static_cast<uint8_t>(body_len);

        return std::move(writer).leak();
    }
}
