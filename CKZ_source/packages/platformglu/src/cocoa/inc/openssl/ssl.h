// PROVISIONAL (created by b003): OpenSSL is linked as a prebuilt library; its headers are not in the project.
// openssl/ssl.h - the subset of the OpenSSL 0.9.8/1.0 SSL API used by CSocket_Cocoa (opaque types only).
#ifndef HEADER_SSL_H
#define HEADER_SSL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

#define SSL_ERROR_WANT_READ     2
#define SSL_ERROR_WANT_WRITE    3

int SSL_library_init(void);
void SSL_load_error_strings(void);
const SSL_METHOD* SSLv23_client_method(void);
SSL_CTX* SSL_CTX_new(const SSL_METHOD* method);
void SSL_CTX_free(SSL_CTX* context);
SSL* SSL_new(SSL_CTX* context);
void SSL_free(SSL* ssl);
int SSL_set_fd(SSL* ssl, int fd);
int SSL_connect(SSL* ssl);
int SSL_shutdown(SSL* ssl);
int SSL_read(SSL* ssl, void* buffer, int length);
int SSL_write(SSL* ssl, const void* buffer, int length);
int SSL_get_error(const SSL* ssl, int result);

#ifdef __cplusplus
}
#endif

#endif
