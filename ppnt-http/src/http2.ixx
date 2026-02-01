export module ppnt.http.http2;

import std;
import ppnt.http.http_types;
import ppnt.traits;
import ppnt.err;
import nghttp2;
import ppnt.io.task;
import ppnt.log;

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
    };

    template<Connection C>
    class Http2Session : public NonCopy {
    private:

        C connection_;
        nghttp2::nghttp2_session *session_{nullptr};
        std::map<int32_t, std::shared_ptr<Http2StreamContext>> streams_{};
        bool should_close_{false};

    public:
        explicit Http2Session(C connection) : connection_{std::move(connection)} {
            init();
        }

        ~Http2Session() {
            if (session_) {
                nghttp2::nghttp2_session_del(session_);
                session_ = nullptr;
            }
        }

        auto request(HttpRequest req) -> io::Task<Result<HttpResponse<Http2Session>>> {
            std::vector<nghttp2::nghttp2_nv> nva{};
            auto &headers = req.headers;
            add_nv(nva, ":method", req.method);
            add_nv(nva, ":scheme", req.protocol);
            add_nv(nva, ":path", req.path);
            if (headers.contains("host")) {
                add_nv(nva, ":authority", *headers.get("host"));
            }
            for (const auto &[k, v] : headers) {
                add_nv(nva, k, v);
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

            while (!ctx->header_complete && !ctx->stream_closed) {
                auto res = co_await drive();
                if (!res) {
                    streams_.erase(stream_id);
                    co_return std::unexpected{res.error()};
                }
            }
            if (ctx->stream_closed && ctx->status_code == -1) {
                streams_.erase(ctx->stream_id);
                co_return make_err_result(std::errc::connection_aborted, "Stream closed");
            }

            auto resp = HttpResponse(this);
            resp.id_ = stream_id;
            resp.set_status({ctx->status_code});
            resp.headers_ = std::move(ctx->headers);
            co_return resp;
        }

        auto body_full(HttpResponse<Http2Session> &resp) -> io::Task<Result<std::vector<uint8_t>>> {
            auto stream_id = resp.id_;
            auto it = streams_.find(stream_id);
            if (it == streams_.end()) {
                co_return make_err_result(std::errc::invalid_argument, std::format("h2 body_full Stream id not found: {}", stream_id));
            }
            auto &ctx = it->second;
            while (!ctx->stream_closed) {
                auto res = co_await drive();
                if (!res) {
                    streams_.erase(stream_id);
                    co_return std::unexpected{res.error()};
                }
            }
            auto data = std::move(ctx->body_buffer);
            streams_.erase(stream_id);
            co_return data;
        }

    private:
        auto init() -> void {
            nghttp2::nghttp2_session_callbacks *callbacks{nullptr};
            nghttp2::nghttp2_session_callbacks_new(&callbacks);

            nghttp2::nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
            nghttp2::nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
            nghttp2::nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
            nghttp2::nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);

            nghttp2::nghttp2_session_client_new(&session_, callbacks, this);
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
            auto *self = static_cast<Http2Session *>(user_data);
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
            auto *self = static_cast<Http2Session *>(user_data);
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
            auto *self = static_cast<Http2Session *>(user_data);
            if (!self->streams_.contains(stream_id)) {
                log::error({"h2 on_stream_close_callback stream id not found"});
                return 0;
            }
            auto &ctx = self->streams_[stream_id];
            ctx->stream_closed = true;
            ctx->header_complete = true;
            return 0;
        }

        static auto on_frame_recv_callback(
            nghttp2::nghttp2_session *session,
            const nghttp2::nghttp2_frame *frame,
            void *user_data
        ) -> int {
            auto *self = static_cast<Http2Session *>(user_data);
            auto stream_id = frame->hd.stream_id;

            if (frame->hd.type == nghttp2::nghttp2_frame_type::NGHTTP2_HEADERS) {
                if (frame->headers.cat == nghttp2::nghttp2_headers_category::NGHTTP2_HCAT_RESPONSE) {
                    if (self->streams_.contains(stream_id)) {
                        auto &ctx = self->streams_[stream_id];
                        ctx->header_complete = true;
                    }
                }
            }
            return 0;
        }
    };

    template<Connection C>
    Http2Session(C) -> Http2Session<C>;
}
