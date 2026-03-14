module;

#include <yyjson.h>

export module yyjson;

export extern "C" inline namespace yyjson {

    using ::yyjson_doc;
    using ::yyjson_val;
    using ::yyjson_alc;

    using ::yyjson_read;
    using ::yyjson_doc_get_root;
    using ::yyjson_doc_free;

    using ::yyjson_mut_doc_new;
    using ::yyjson_mut_doc_free;
    using ::yyjson_write;

    using ::yyjson_obj_get;
    using ::yyjson_arr_get;
    using ::yyjson_get_int;
    using ::yyjson_get_bool;
    using ::yyjson_get_len;
}