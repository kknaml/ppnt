
module;

#include <openssl/ssl.h>
#include <openssl/crypto.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/mem.h>


export module boringssl;

export namespace boringssl {
    // Types
    using ::SSL;
    using ::SSL_CTX;
    using ::BIO;
    using ::BIO_METHOD;
    using ::X509;
    using ::X509_NAME;
    using ::EVP_PKEY;
    using ::EVP_PKEY_CTX;
    using ::EVP_MD;
    using ::EVP_MD_CTX;

    // SSL Functions
    using ::SSL_CTX_new;
    using ::SSL_CTX_free;
    using ::SSL_new;
    using ::SSL_free;
    using ::SSL_set_fd;
    using ::SSL_connect;
    using ::SSL_accept;
    using ::SSL_read;
    using ::SSL_write;
    using ::SSL_get_error;
    using ::SSL_shutdown;
    using ::TLS_method;
    using ::SSL_CTX_set_grease_enabled;

    using ::TLS_client_method;
    using ::TLS_server_method;
    using ::SSL_CTX_set_min_proto_version;
    using ::SSL_do_handshake;
    using ::SSL_set_tlsext_host_name;
    using ::BIO_new_bio_pair;
    using ::SSL_set_bio;
    using ::BIO_ctrl_pending;
    using ::SSL_set_connect_state;
    using ::SSL_get_ex_data;
    using ::SSL_get_ex_new_index;
    using ::SSL_set_ex_data;
    using ::SSL_set_client_hello_interceptor;

    // BIO Functions
    using ::BIO_new;
    using ::BIO_free;
    using ::BIO_s_mem;
    using ::BIO_read;
    using ::BIO_write;
    using ::BIO_ctrl;
    using ::BIO_s_file;
    using ::BIO_new_fp;
    using ::BIO_new_mem_buf;

    // CRYPTO Functions
    using ::CRYPTO_library_init;
    using ::OPENSSL_malloc;
    using ::OPENSSL_free;

    // EVP Functions
    using ::EVP_PKEY_new;
    using ::EVP_PKEY_free;
    using ::EVP_sha256;
    using ::EVP_DigestSignInit;
    using ::EVP_DigestSignUpdate;
    using ::EVP_DigestSignFinal;
    using ::EVP_DigestVerifyInit;
    using ::EVP_DigestVerifyUpdate;
    using ::EVP_DigestVerifyFinal;

    // X509 Functions
    using ::X509_get_subject_name;
    using ::X509_NAME_oneline;
    using ::d2i_X509;
    using ::i2d_X509;

    // ERR Functions
    using ::ERR_get_error;
    using ::ERR_error_string_n;
    using ::ERR_print_errors_fp;
    using ::ERR_clear_error;
    using ::ERR_peek_error;

    // Constants
    constexpr inline int SSL_ERROR_NONE_ = SSL_ERROR_NONE;
    constexpr inline int SSL_ERROR_SSL_ = SSL_ERROR_SSL;
    constexpr inline int SSL_ERROR_WANT_READ_ = SSL_ERROR_WANT_READ;
    constexpr inline int SSL_ERROR_WANT_WRITE_ = SSL_ERROR_WANT_WRITE;
    constexpr inline int SSL_ERROR_WANT_X509_LOOKUP_ = SSL_ERROR_WANT_X509_LOOKUP;
    constexpr inline int SSL_ERROR_SYSCALL_ = SSL_ERROR_SYSCALL;
    constexpr inline int SSL_ERROR_ZERO_RETURN_ = SSL_ERROR_ZERO_RETURN;
    constexpr inline int SSL_ERROR_WANT_CONNECT_ = SSL_ERROR_WANT_CONNECT;
    constexpr inline int SSL_ERROR_WANT_ACCEPT_ = SSL_ERROR_WANT_ACCEPT;

    constexpr inline int SSL_FILETYPE_PEM_ = SSL_FILETYPE_PEM;
    constexpr inline int SSL_FILETYPE_ASN1_ = SSL_FILETYPE_ASN1;

    constexpr inline int X509_V_OK_ = X509_V_OK;

    constexpr inline int EVP_PKEY_RSA_ = EVP_PKEY_RSA;
    constexpr inline int EVP_PKEY_EC_ = EVP_PKEY_EC;

    constexpr inline int tls1_2_version = TLS1_2_VERSION;
}
