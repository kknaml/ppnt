export module ppnt.http.http2;

import std;
import ppnt.common;
import ppnt.http.http_types;
import nghttp2;
import ppnt.io.task;
import ppnt.http.session_key;
import ppnt.io.runtime;

export namespace ppnt::http {

    template<Connection C>
    class Http2Session;

    struct Http2StreamContext {
        int32_t stream_id{-1};
        int status_code{-1};
        HttpHeaderList headers{};
        std::vector<uint8_t> body_buffer{};
        bool header_complete{false};
        bool stream_closed{false};

        std::function<void(bool)> header_waker{};
        std::function<void(std::vector<uint8_t>)> body_waker{};
    };

    template<Connection C>
    class Http2Session : public NonCopy {
    private:

        C connection_;
        nghttp2::nghttp2_session *session_{nullptr};
        std::map<int32_t, std::shared_ptr<Http2StreamContext>> streams_{};
        bool should_close_{false};

        SessionKey session_key_;
        io::JoinHandle<void> back_task_{nullptr};

        struct Nghttp2UserData {
            Http2Session* self_ptr;
        };
        std::unique_ptr<Nghttp2UserData> user_data_block_;

    public:
        explicit Http2Session(C connection, SessionKey session_key)
        : connection_{std::move(connection)}, session_key_(std::move(session_key)) {
            user_data_block_ = std::make_unique<Nghttp2UserData>(Nghttp2UserData{this});
            init();
        }

        Http2Session(Http2Session &&other) noexcept :
            connection_(std::move(other.connection_)),
            session_(std::exchange(other.session_, nullptr)),
            streams_(std::move(other.streams_)),
            should_close_(other.should_close_),
            session_key_(std::move(other.session_key_)),
            back_task_(std::move(other.back_task_)),
            user_data_block_(std::move(other.user_data_block_))
        {
            if (user_data_block_) {
                user_data_block_->self_ptr = this;
            }
        }

        auto operator=(Http2Session &&other) noexcept -> Http2Session & {
            if (this != &other) {
                this->connection_ = std::move(other.connection_);
                this->session_ = std::exchange(other.session_, nullptr);
                this->streams_ = std::move(other.streams_);
                this->should_close_ = other.should_close_;
                this->session_key_ = std::move(other.session_key_);
                this->back_task_ = std::move(other.back_task_);

                this->user_data_block_ = std::move(other.user_data_block_);
                if (this->user_data_block_) {
                    this->user_data_block_->self_ptr = this;
                }
            }
            return *this;
        }

        ~Http2Session() {
            if (session_) {
                nghttp2::nghttp2_session_del(session_);
                session_ = nullptr;
            }
        }

        auto request(HttpRequest req) -> io::Task<Result<HttpResponse<Http2Session>>> {
            if (!back_task_) {
                back_task_ = io::spawn(background_driver());
            }
            std::vector<nghttp2::nghttp2_nv> nva{};
            std::deque<std::string> string_keeper{};

            add_nv(nva, ":method", req.method);
            add_nv(nva, ":scheme", req.url.scheme());
            std::string path_str = req.url.path().empty() ? "/" : std::string{req.url.path()};
            string_keeper.push_back(std::move(path_str));
            add_nv(nva, ":path", string_keeper.back());

            auto &headers = req.headers;
            bool has_host = false;
            if (headers.contains("host")) {
                add_nv(nva, ":authority", *headers.get("host"));
            } else {
                string_keeper.push_back(std::string{req.url.host()});
                add_nv(nva, ":authority", string_keeper.back());
            }
            for (const auto &[k, v] : headers) {
                if (ignore_case_equal(k, "host") ||
                    ignore_case_equal(k, "connection") ||
                    ignore_case_equal(k, "keep-alive") ||
                    ignore_case_equal(k, "upgrade")) {
                    continue;
                    }
                std::string lower_k{k};
                std::transform(lower_k.begin(), lower_k.end(), lower_k.begin(),
                               [](unsigned char c){ return std::tolower(c); });

                string_keeper.push_back(std::move(lower_k));
                add_nv(nva, string_keeper.back(), v);
            }

            auto stream_id = nghttp2::nghttp2_submit_request(
                session_,
                nullptr,
                nva.data(),
                nva.size(),
                nullptr,
                nullptr
            );
            if (stream_id < 0) {
                co_return make_err_result(std::errc::invalid_argument, "nghttp2 submit failed");
            }

            auto ctx = std::make_shared<Http2StreamContext>();
            ctx->stream_id = stream_id;
            streams_[stream_id] = ctx;

            auto write_res = co_await flush_writes();
            if (!write_res) {
                streams_.erase(stream_id);
                co_return make_err_result(std::errc::io_error, "Failed to send request");
            }

            auto success = co_await io::suspend_coroutine<bool>([ctx](auto continuation) {
                ctx->header_waker = std::move(continuation);
            });

            if (!success || (ctx->stream_closed && ctx->status_code == -1)) {
                streams_.erase(stream_id);
                co_return make_err_result(std::errc::connection_aborted, "Stream closed or failed");
            }

            // while (!ctx->header_complete && !ctx->stream_closed) {
            //     auto res = co_await drive();
            //     if (!res) {
            //         streams_.erase(stream_id);
            //         co_return std::unexpected{res.error()};
            //     }
            // }


            auto resp = HttpResponse(this);
            resp.set_id(stream_id);
            resp.set_status({ctx->status_code});
            resp.head_.headers = std::move(ctx->headers);
            co_return resp;
        }

        template<typename Session>
        auto body_full(HttpResponse<Session> &resp) -> io::Task<Result<std::vector<uint8_t>>> {
            auto stream_id = resp.get_id();
            auto it = streams_.find(stream_id);
            if (it == streams_.end()) {
                co_return make_err_result(std::errc::invalid_argument, "Stream id not found");
            }

            auto &ctx = it->second;

            if (ctx->stream_closed) {
                auto data = std::move(ctx->body_buffer);
                streams_.erase(stream_id);
                co_return data;
            }

            auto data = co_await io::suspend_coroutine<std::vector<uint8_t>>([&ctx](auto resume_cb) {
                ctx->body_waker = std::move(resume_cb);
            });

            streams_.erase(stream_id);
            co_return data;
        }

        [[nodiscard]]
       auto is_valid() const -> bool {
            if (should_close_ || !session_) return false;
            return nghttp2::nghttp2_session_want_read(session_) != 0 ||
                   nghttp2::nghttp2_session_want_write(session_) != 0;
        }

        [[nodiscard]]
        auto get_http_version() const -> Version {
            return Version::HTTP_2;
        }

        [[nodiscard]]
        auto get_session_key() const -> const SessionKey & {
            return session_key_;
        }

        auto close() -> void {
            should_close_ = true;
            connection_.close();
        }

    private:
        auto init() -> void {
            nghttp2::nghttp2_session_callbacks *callbacks{nullptr};
            nghttp2::nghttp2_session_callbacks_new(&callbacks);

            nghttp2::nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
            nghttp2::nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
            nghttp2::nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
            nghttp2::nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);

            // nghttp2::nghttp2_session_client_new(&session_, callbacks, this);

            nghttp2::nghttp2_session_client_new(&session_, callbacks, user_data_block_.get());

            nghttp2::nghttp2_session_callbacks_del(callbacks);

            nghttp2::nghttp2_submit_settings(session_, nghttp2::nghttp2_flag::NGHTTP2_FLAG_NONE, nullptr, 0);
        }

        auto add_nv(
            std::vector<nghttp2::nghttp2_nv> &nva,
            std::string_view name,
            std::string_view value
        ) -> void {
            auto nv = nghttp2::nghttp2_nv{};
            nv.name = reinterpret_cast<uint8_t *>(const_cast<char *>(name.data()));
            nv.namelen = name.size();
            nv.value = reinterpret_cast<uint8_t *>(const_cast<char *>(value.data()));
            nv.valuelen = value.size();
            nv.flags = nghttp2::nghttp2_flag::NGHTTP2_FLAG_NONE;
            nva.push_back(std::move(nv));
        }

        auto drive() -> io::Task<Result<Unit>> {
            auto keep_looping = true;
            while (keep_looping) {
                keep_looping = false;
                while (nghttp2::nghttp2_session_want_write(session_)) {
                    const uint8_t *data;
                    auto len = nghttp2::nghttp2_session_mem_send(session_, &data);
                    if (len == 0) break;
                    if (len < 0) {
                        co_return make_err_result(std::errc::io_error, "nghttp2 send failed");
                    }
                    auto write_res = co_await connection_.write(std::span{data, static_cast<size_t>(len)});
                    if (!write_res) co_return std::unexpected{write_res.error()};

                    keep_looping = true;
                }

                if (nghttp2::nghttp2_session_want_read(session_)) {

                }
            }

            while (nghttp2::nghttp2_session_want_write(session_)) {
                const uint8_t *data;
                auto len = nghttp2::nghttp2_session_mem_send(session_, &data);
                if (len <= 0) break;
                co_await connection_.write(std::span{data, static_cast<size_t>(len)});
            }

            if (nghttp2::nghttp2_session_want_read(session_)) {
                std::array<uint8_t, 4096> buffer{};
                auto read_res = co_await connection_.read(buffer);
                if (!read_res) co_return std::unexpected{read_res.error()};

                auto n = *read_res;
                if (n > 0) {
                    auto len = nghttp2::nghttp2_session_mem_recv(session_, buffer.data(), n);
                    if (len < 0) co_return make_err_result(std::errc::io_error, "h2 recv failed");

                    while (nghttp2::nghttp2_session_want_write(session_)) {
                        const uint8_t *data;
                        auto wlen = nghttp2::nghttp2_session_mem_send(session_, &data);
                        if (wlen <= 0) break;
                        auto wres = co_await connection_.write(std::span{data, static_cast<size_t>(wlen)});
                        if (!wres) co_return std::unexpected{wres.error()};
                    }
                } else if (n == 0) {
                    co_return make_err_result(std::errc::not_connected, "Peer closed");
                }
            }
            co_return {};


            // while (nghttp2::nghttp2_session_want_write(session_)) {
            //     const uint8_t *data;
            //     auto len = nghttp2::nghttp2_session_mem_send(session_, &data);
            //     if (len == 0) break;
            //     if (len < 0) {
            //         co_return make_err_result(std::errc::io_error, "nghttp2_session_mem_send failed");
            //     }
            //
            //     auto write_res = co_await connection_.write(std::span{data, static_cast<size_t>(len)});
            //     if (!write_res) {
            //         co_return std::unexpected{write_res.error()};
            //     }
            // }
            // if (nghttp2::nghttp2_session_want_read(session_)) {
            //     std::array<uint8_t, 4096> buffer{};
            //     auto read_res = co_await connection_.read(buffer);
            //     if (!read_res) {
            //         co_return std::unexpected{read_res.error()};
            //     }
            //     auto n = *read_res;
            //     if (n > 0) {
            //         auto len = nghttp2::nghttp2_session_mem_recv(session_, buffer.data(), static_cast<size_t>(n));
            //         if (len < 0) {
            //             co_return make_err_result(std::errc::io_error, "nghttp2_session_mem_send failed");
            //         }
            //     } else if (n == 0) {
            //
            //     }
            // }
            // co_return {};
        }

        auto flush_writes() -> io::Task<Result<Unit>> {
            while (nghttp2::nghttp2_session_want_write(session_)) {
                const uint8_t *data;
                auto len = nghttp2::nghttp2_session_mem_send(session_, &data);
                if (len <= 0) break;
                auto write_res = co_await connection_.write(std::span{data, static_cast<size_t>(len)});
                if (!write_res) co_return std::unexpected{write_res.error()};
            }
            co_return {};
        }

        auto background_driver() -> io::Task<> {
            std::array<uint8_t, 4096> buffer{};
            while (!should_close_) {
                auto read_res = co_await connection_.read(buffer);
                if (!read_res || *read_res == 0) {
                    should_close_ = true;
                    break;
                }

                auto rv = nghttp2::nghttp2_session_mem_recv(session_, buffer.data(), *read_res);
                if (rv < 0) {
                    should_close_ = true;
                    break;
                }

                co_await flush_writes();
            }

            for (auto& [id, ctx] : streams_) {
                ctx->stream_closed = true;
                if (ctx->header_waker) {
                    auto waker = std::move(ctx->header_waker);
                    ctx->header_waker = nullptr;
                    waker(false);
                }
                if (ctx->body_waker) {
                    auto waker = std::move(ctx->body_waker);
                    ctx->body_waker = nullptr;
                    waker({});
                }
            }
            streams_.clear();
        }

    public:
        static auto on_header_callback(
            nghttp2::nghttp2_session *session,
            const nghttp2::nghttp2_frame *frame,
            const uint8_t *name,
            size_t name_len,
            const uint8_t *value,
            size_t value_len,
            uint8_t flags,
            void *user_data
        ) -> int {
           //  auto *self = static_cast<Http2Session *>(user_data);
            auto *user_data_block = static_cast<Nghttp2UserData *>(user_data);
            auto *self = user_data_block->self_ptr;
            if (frame->hd.type != nghttp2::nghttp2_frame_type::NGHTTP2_HEADERS) return 0;
            auto stream_id = frame->hd.stream_id;
            if (!self->streams_.contains(stream_id)) {
                log::error({"h2 on_header_callback stream id not found"});
                return 0;
            }
            auto &ctx = self->streams_[stream_id];
            auto key = std::string_view{reinterpret_cast<const char *>(name), name_len};
            auto val = std::string_view{reinterpret_cast<const char *>(value), value_len};
            if (key == ":status") {
                std::from_chars(val.data(), val.data() + val.size(), ctx->status_code);
            } else if (!key.starts_with(":")) {
                ctx->headers.add(key, val);
            } else {
                log::warn({"h2 on_header_callback unknown header: {}: {}"}, key, val);
            }
            return 0;
        }
        static auto on_data_chunk_recv_callback(
            nghttp2::nghttp2_session *session,
            uint8_t flag,
            int32_t stream_id,
            const uint8_t *data,
            size_t data_len,
            void *user_data
        ) -> int {
            auto *user_data_block = static_cast<Nghttp2UserData *>(user_data);
            auto *self = user_data_block->self_ptr;
            if (!self->streams_.contains(stream_id)) {
                log::error({"h2 on_data_chunk_recv_callback stream id not found"});
                return 0;
            }
            auto &ctx = self->streams_[stream_id];
            ctx->body_buffer.insert(ctx->body_buffer.end(), data, data + data_len);
            nghttp2::nghttp2_session_consume_stream(session, stream_id, data_len);
            nghttp2::nghttp2_session_consume_connection(session, data_len);
            return 0;
        }

        static auto on_stream_close_callback(
            nghttp2::nghttp2_session *session,
            int32_t stream_id,
            uint32_t error_code,
            void *user_data
        )-> int {
            auto *user_data_block = static_cast<Nghttp2UserData *>(user_data);
            auto *self = user_data_block->self_ptr;
            if (!self->streams_.contains(stream_id)) {
                log::error({"h2 on_stream_close_callback stream id not found"});
                return 0;
            }
            auto &ctx = self->streams_[stream_id];
            if (error_code != 0) {
                log::warn({"h2 stream {} reset by peer, error_code: {}"}, stream_id, error_code);
            }
            ctx->stream_closed = true;
            ctx->header_complete = true;
            if (ctx->header_waker) {
                auto waker = std::move(ctx->header_waker);
                ctx->header_waker = nullptr;
                waker(false);
            }
            if (ctx->body_waker) {
                auto waker = std::move(ctx->body_waker);
                ctx->body_waker = nullptr;
                waker(std::move(ctx->body_buffer));
            }

            return 0;
        }

        static auto on_frame_recv_callback(
            nghttp2::nghttp2_session *session,
            const nghttp2::nghttp2_frame *frame,
            void *user_data
        ) -> int {
            auto *user_data_block = static_cast<Nghttp2UserData *>(user_data);
            auto *self = user_data_block->self_ptr;
            auto stream_id = frame->hd.stream_id;

            if (frame->hd.type == nghttp2::nghttp2_frame_type::NGHTTP2_HEADERS) {
                if (frame->headers.cat == nghttp2::nghttp2_headers_category::NGHTTP2_HCAT_RESPONSE) {
                    if (self->streams_.contains(stream_id)) {
                        auto &ctx = self->streams_[stream_id];
                        ctx->header_complete = true;

                        if (ctx->header_waker) {
                            auto waker = std::move(ctx->header_waker);
                            ctx->header_waker = nullptr;
                            waker(true);
                        }
                    }
                }
            }
            return 0;
        }
    };

    template<Connection C>
    Http2Session(C) -> Http2Session<C>;
}
