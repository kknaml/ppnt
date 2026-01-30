module;

#include <llhttp.h>

export module llhttp;

export namespace llhttp {

    using ::llhttp_t;
    using ::llhttp_settings_t;

    using ::llhttp_errno;
    using ::llhttp_flags;
    using ::llhttp_lenient_flags;
    using ::llhttp_type;
    using ::llhttp_finish;
    using ::llhttp_method;
    using ::llhttp_status;
    using ::llhttp_settings_s;
    using ::llhttp_data_cb;
    using ::llhttp_cb;
    using ::llhttp_settings_s;

    using ::llhttp_init;
    using ::llhttp_alloc;
    using ::llhttp_free;
    using ::llhttp_get_type;
    using ::llhttp_get_http_major;
    using ::llhttp_get_http_minor;
    using ::llhttp_get_method;
    using ::llhttp_get_status_code;
    using ::llhttp_get_upgrade;
    using ::llhttp_reset;
    using ::llhttp_settings_init;
    using ::llhttp_execute;
    using ::llhttp_finish;
    using ::llhttp_message_needs_eof;
    using ::llhttp_should_keep_alive;
    using ::llhttp_pause;
    using ::llhttp_resume;
    using ::llhttp_resume_after_upgrade;
    using ::llhttp_get_errno;
    using ::llhttp_get_error_reason;
    using ::llhttp_set_error_reason;
    using ::llhttp_get_error_pos;
    using ::llhttp_errno_name;
    using ::llhttp_method_name;
    using ::llhttp_status_name;
    using ::llhttp_set_lenient_headers;
    using ::llhttp_set_lenient_chunked_length;
    using ::llhttp_set_lenient_keep_alive;
    using ::llhttp_set_lenient_transfer_encoding;
    using ::llhttp_set_lenient_version;
    using ::llhttp_set_lenient_data_after_close;
    using ::llhttp_set_lenient_optional_lf_after_cr;
    using ::llhttp_set_lenient_optional_cr_before_lf;
    using ::llhttp_set_lenient_optional_crlf_after_chunk;
    using ::llhttp_set_lenient_spaces_after_chunk_size;

}
