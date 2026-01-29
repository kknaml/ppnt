
module;

#include <nghttp2/nghttp2.h>

export module nghttp2;

export namespace nghttp2 {
    
    // Types
    using ::nghttp2_session;
    using ::nghttp2_session_callbacks;
    using ::nghttp2_option;
    using ::nghttp2_hd_deflater;
    using ::nghttp2_hd_inflater;
    using ::nghttp2_stream;
    using ::nghttp2_nv;
    using ::nghttp2_settings_entry;
    using ::nghttp2_data_provider;
    using ::nghttp2_data_source;
    using ::nghttp2_info;
    using ::nghttp2_rcbuf;
    using ::nghttp2_vec;

    // Enums
    using ::nghttp2_error;
    using ::nghttp2_nv_flag;
    using ::nghttp2_frame_type;
    using ::nghttp2_flag;
    using ::nghttp2_settings_id;
    // using ::nghttp2_stream_state;
    using ::nghttp2_headers_category;

    // Callback types
    using ::nghttp2_send_callback;
    using ::nghttp2_recv_callback;
    using ::nghttp2_on_frame_recv_callback;
    using ::nghttp2_on_invalid_frame_recv_callback;
    using ::nghttp2_on_data_chunk_recv_callback;
    using ::nghttp2_before_frame_send_callback;
    using ::nghttp2_on_frame_send_callback;
    using ::nghttp2_on_frame_not_send_callback;
    using ::nghttp2_on_stream_close_callback;
    using ::nghttp2_on_begin_headers_callback;
    using ::nghttp2_on_header_callback;
    using ::nghttp2_select_padding_callback;
    using ::nghttp2_data_source_read_callback;

    // Functions - Session setup
    using ::nghttp2_session_callbacks_new;
    using ::nghttp2_session_callbacks_del;
    using ::nghttp2_session_callbacks_set_send_callback;
    using ::nghttp2_session_callbacks_set_recv_callback;
    using ::nghttp2_session_callbacks_set_on_frame_recv_callback;
    using ::nghttp2_session_callbacks_set_on_invalid_frame_recv_callback;
    using ::nghttp2_session_callbacks_set_on_data_chunk_recv_callback;
    using ::nghttp2_session_callbacks_set_before_frame_send_callback;
    using ::nghttp2_session_callbacks_set_on_frame_send_callback;
    using ::nghttp2_session_callbacks_set_on_frame_not_send_callback;
    using ::nghttp2_session_callbacks_set_on_stream_close_callback;
    using ::nghttp2_session_callbacks_set_on_begin_headers_callback;
    using ::nghttp2_session_callbacks_set_on_header_callback;
    using ::nghttp2_session_callbacks_set_select_padding_callback;
    // using ::nghttp2_session_callbacks_set_data_source_read_callback;
    
    using ::nghttp2_session_client_new;
    using ::nghttp2_session_server_new;
    using ::nghttp2_session_del;
    using ::nghttp2_session_send;
    using ::nghttp2_session_mem_recv;
    using ::nghttp2_session_resume_data;
    using ::nghttp2_session_want_read;
    using ::nghttp2_session_want_write;
    using ::nghttp2_session_get_stream_user_data;
    using ::nghttp2_session_set_stream_user_data;
    using ::nghttp2_session_terminate_session;

    // Functions - Submission
    using ::nghttp2_submit_request;
    using ::nghttp2_submit_response;
    using ::nghttp2_submit_headers;
    using ::nghttp2_submit_data;
    using ::nghttp2_submit_priority;
    using ::nghttp2_submit_rst_stream;
    using ::nghttp2_submit_settings;
    using ::nghttp2_submit_ping;
    using ::nghttp2_submit_goaway;
    using ::nghttp2_submit_window_update;

    // Functions - Options
    using ::nghttp2_option_new;
    using ::nghttp2_option_del;
    using ::nghttp2_option_set_no_auto_window_update;
    using ::nghttp2_option_set_peer_max_concurrent_streams;

    // Functions - Utils
    using ::nghttp2_version;
    using ::nghttp2_strerror;
    using ::nghttp2_hd_deflate_new;
    using ::nghttp2_hd_deflate_del;
    using ::nghttp2_hd_inflate_new;
    using ::nghttp2_hd_inflate_del;
}
