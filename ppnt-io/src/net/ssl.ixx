
export module ppnt.net.ssl;

import std;
import ppnt.net.tcp_stream;
import boringssl;
import ppnt.traits;
import ppnt.err;
import ppnt.io.task;
import ppnt.log;
import ppnt.net.tls;

export namespace ppnt::net {

    struct SslFree {
        auto operator()(boringssl::SSL *ptr) -> void;

        auto operator()(boringssl::SSL_CTX *ptr) -> void {
            boringssl::SSL_CTX_free(ptr);
        }
        auto operator()(boringssl::BIO *ptr) -> void {
            boringssl::BIO_free(ptr);
        }
    };

    using UniqueSsl = std::unique_ptr<boringssl::SSL, SslFree>;
    using UniqueCtx = std::unique_ptr<boringssl::SSL_CTX, SslFree>;
    using UniqueBio = std::unique_ptr<boringssl::BIO, SslFree>;

    class TlsContext {
    private:
        UniqueCtx ctx_;
        ClientHelloSpecFactory *spec_factory_;

    public:
        explicit TlsContext(boringssl::SSL_CTX *ctx, ClientHelloSpecFactory *spec_factory)
        : ctx_(ctx), spec_factory_(spec_factory) {}

        TlsContext(TlsContext &&other) noexcept
        : ctx_(std::move(other.ctx_)), spec_factory_(std::exchange(other.spec_factory_, nullptr)) {}

        TlsContext(const TlsContext &other) = delete;

        auto operator=(const TlsContext &) = delete;

        auto operator=(TlsContext &&other) noexcept -> TlsContext & {
            if (this != &other) {
                this->ctx_ = std::move(other.ctx_);
                this->spec_factory_ = std::exchange(other.spec_factory_, nullptr);
            }
            return *this;
        }

        static auto client(ClientHelloSpecFactory *spec_factory = nullptr) -> Result<TlsContext> {
            auto *method = boringssl::TLS_client_method();
            auto *ctx = boringssl::SSL_CTX_new(method);
            if (!ctx) {
                return std::unexpected{Error{std::make_error_code(std::errc::protocol_error)}};
            }
            if (spec_factory != nullptr) {
                // auto spec = spec_factory->get_tls_spec();
                // TODO apply spec
            }
            if (boringssl::SSL_CTX_set_cipher_list(ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH") != 1) {
                return make_err_result(std::errc::wrong_protocol_type, "SSL_CTX_set_cipher_list ALL failed");
            }
            boringssl::SSL_CTX_set_min_proto_version(ctx,boringssl::tls1_2_version);
            boringssl::SSL_CTX_set_max_proto_version(ctx,boringssl::tls1_3_version);
            boringssl::SSL_CTX_set_grease_enabled(ctx, 1);
            return TlsContext(ctx, spec_factory);
        }

        [[nodiscard]]
        auto native_handle() const -> const boringssl::SSL_CTX * {
            return ctx_.get();
        }

        [[nodiscard]]
        auto native_handle() ->  boringssl::SSL_CTX * {
            return ctx_.get();
        }

        [[nodiscard]]
        auto get_spec_factory() const -> ClientHelloSpecFactory * {
            return spec_factory_;
        }
    };

    auto is_h2_negotiated(boringssl::SSL *ssl) -> bool;

    class TlsStream : public NonCopy {
    private:
        TcpStream inner_;
        UniqueSsl ssl_;
        UniqueBio network_bio_;
        std::vector<uint8_t> io_buffer_;

        TlsStream(TcpStream inner, UniqueSsl ssl, boringssl::BIO *network_bio)
            : inner_(std::move(inner)), ssl_(std::move(ssl)), network_bio_(network_bio) {
            io_buffer_.resize(16384);
        }

    public:

        TlsStream(TlsStream &&other) noexcept : inner_(std::move(other.inner_)), ssl_(std::move(other.ssl_)),
            network_bio_(std::move(other.network_bio_)), io_buffer_(std::move(other.io_buffer_)) {}

        auto operator=(TlsStream &&other) noexcept -> TlsStream & {
            if (this != &other) {
                this->inner_ = std::move(other.inner_);
                this->ssl_ = std::move(other.ssl_);
                this->network_bio_ = std::move(other.network_bio_);
                this->io_buffer_ = std::move(other.io_buffer_);
            }
            return *this;
        }

        static auto connect(TcpStream inner, TlsContext &ctx, std::string_view host_name, bool handshake = true) -> io::Task<Result<TlsStream>> {
            auto *raw_ssl = boringssl::SSL_new(ctx.native_handle());
            if (!raw_ssl) {
                co_return std::unexpected{Error{std::make_error_code(std::errc::not_enough_memory), "tls connect"}};
            }
            {
                static const std::vector<uint8_t> alpn = {
                    0x02, 'h', '2',
                    0x08, 'h', 't', 't', 'p', '/', '1', '.', '1'
                };
                auto ret = boringssl::SSL_set_alpn_protos(raw_ssl, alpn.data(), alpn.size());
                if (ret != 0) {
                    co_return make_err_result(std::errc::invalid_argument, "SSL_set_alpn_protos");
                }
            }
            // auto *spec = create_hello_spec_factory(make_test_spec);
            auto *spec = ctx.get_spec_factory();
            boringssl::SSL_set_ex_data(raw_ssl, detail::get_tls_spec_slot(), static_cast<void *>(spec));
            boringssl::SSL_set_client_hello_interceptor(raw_ssl, my_client_hello_interceptor);
            boringssl::SSL_set_connect_state(raw_ssl);
            auto ssl = UniqueSsl{raw_ssl};
            std::string host{host_name};
            boringssl::SSL_set_tlsext_host_name(raw_ssl, host.c_str());
            boringssl::BIO *internal_bio = nullptr, *network_bio = nullptr;
            if (!boringssl::BIO_new_bio_pair(&internal_bio, 16384,  &network_bio, 16384)) {
                co_return std::unexpected{Error{std::make_error_code(std::errc::io_error), "tls connect: cant create bio pare"}};
            }
            boringssl::SSL_set_bio(raw_ssl, internal_bio, internal_bio);
            auto tls = TlsStream(std::move(inner), std::move(ssl), network_bio);
            if (handshake) {
                auto res = co_await tls.handshake();
                if (!res) co_return std::unexpected{res.error()};
            }
            co_return tls;
        }

        auto handshake() -> io::Task<Result<Unit>> {
            auto op = [&](boringssl::SSL *ssl) {
                return boringssl::SSL_do_handshake(ssl);
            };
            auto res = co_await run_ssl_op(op);
            if (res) {
                const unsigned char *alpn{nullptr};
                unsigned int alpn_len{0};
                boringssl::SSL_get0_alpn_selected(ssl_.get(), &alpn, &alpn_len);
                if (alpn_len > 0) {
                    auto str = std::string_view{reinterpret_cast<const char *>(alpn), alpn_len};
                    log::info({"Negotiated Protocol: {}"}, str);
                } else {
                    log::info({"Negotiated Protocol not found"});
                }
                co_return {};
            }
            co_return std::unexpected{res.error()};
        }

        auto is_h2() const -> bool {
            return is_h2_negotiated(ssl_.get());
        }

        auto read(std::span<uint8_t> buf) -> io::Task<Result<size_t>> {
            auto op = [&](boringssl::SSL *ssl) {
                return boringssl::SSL_read(ssl, buf.data(), static_cast<int>(buf.size()));
            };
            co_return co_await run_ssl_op(op);
        }

        auto write(std::span<const uint8_t> buf) -> io::Task<Result<size_t>> {
            auto op = [&](boringssl::SSL *ssl) {
                return boringssl::SSL_write(ssl, buf.data(), static_cast<int>(buf.size()));
            };
            co_return co_await run_ssl_op(op);
        }

        auto shutdown() -> io::Task<Result<Unit>> {
            auto op = [&](boringssl::SSL *ssl) {
                return boringssl::SSL_shutdown(ssl);
            };
            auto res = co_await run_ssl_op(op);
            if (!res) co_return std::unexpected{res.error()};

            inner_.close();
            co_return {};
        }

        auto is_alive() const noexcept -> bool {
            if (!inner_.is_alive()) return false;
            if (boringssl::SSL_get_shutdown(ssl_.get()) & boringssl::SSL_RECEIVED_SHUTDOWN_){
                return false;
            }
            return true;
        }

        auto close() -> void {
            inner_.close();
        }

    private:

        template<typename Op>
        auto run_ssl_op(Op &&op) -> io::Task<Result<int>> {
            int ret = 0;
            while (true) {
                ret = op(ssl_.get());
                if (ret > 0) {
                    auto flush_res = co_await flush_bio_to_tcp();
                    if (!flush_res) co_return std::unexpected{flush_res.error()};
                    co_return ret;
                }

                auto ssl_err = boringssl::SSL_get_error(ssl_.get(), ret);
                auto flush_res = co_await flush_bio_to_tcp();
                if (!flush_res) co_return std::unexpected{flush_res.error()};

                if (ssl_err == boringssl::SSL_ERROR_WANT_READ_) {
                    auto fill_res = co_await fill_bio_from_tcp();
                    if (!fill_res) co_return std::unexpected{fill_res.error()};
                    if (*fill_res == 0) {
                        co_return std::unexpected{Error{std::make_error_code(std::errc::connection_aborted)}};
                    }
                    continue;
                } else if (ssl_err == boringssl::SSL_ERROR_WANT_WRITE_) {
                    continue;
                } else if (ssl_err == boringssl::SSL_ERROR_ZERO_RETURN_) {
                    co_return 0; // EOF
                } else {
                    std::string error_msg;
                    while (auto err = boringssl::ERR_get_error()) {
                        char buf[256];
                        boringssl::ERR_error_string_n(err, buf, sizeof(buf));
                        if (!error_msg.empty()) error_msg += "; ";
                        error_msg += buf;
                    }
                    co_return std::unexpected{
                        Error{
                            std::make_error_code(std::errc::protocol_error),
                            std::format("ssl_err is {}, msg: {}", ssl_err, error_msg)
                        }
                    };
                }
            }
        }

        auto flush_bio_to_tcp() -> io::Task<Result<Unit>> {
            while (true) {
                auto pending = boringssl::BIO_ctrl_pending(network_bio_.get());
                if (pending == 0) break;

                auto n = boringssl::BIO_read(network_bio_.get(), io_buffer_.data(), static_cast<int>(io_buffer_.size()));
                if (n <= 0) break;

                auto res = co_await inner_.write_all(std::span{io_buffer_}.first(n));
                if (!res) co_return std::unexpected{res.error()};
            }
            co_return {};
        }

        auto fill_bio_from_tcp() -> io::Task<Result<size_t>> {
            auto res = co_await inner_.read(std::span{io_buffer_});
            if (!res) co_return std::unexpected{res.error()};
            auto n = *res;
            if (n > 0) {
                auto written = boringssl::BIO_write(network_bio_.get(), io_buffer_.data(), n);
                if (written != n) [[unlikely]] {
                    log::error({"CRITICAL DATA LOSS: Read {} bytes from TCP but only wrote {} to BIO"}, n, written);
                }
            }
            co_return n;
        }
    };


}
