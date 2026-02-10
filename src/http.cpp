// ============================================================
// http.cpp — lightweight HTTP/HTTPS client for Spry
//
// Features:
//   - HTTP/1.1 with chunked transfer-encoding
//   - HTTPS via SChannel (Windows) or dlopen'd OpenSSL (Linux/macOS)
//   - Non-blocking: runs request on a worker thread
//   - Lua coroutine-friendly API
//
// TLS backends:
//   Windows: native SChannel (no external dependency)
//   Linux/macOS: OpenSSL loaded at runtime via dlopen
//   HTML5: no TLS support
// ============================================================

#include "http.h"
#include "array.h"
#include "luax.h"
#include "prelude.h"
#include "strings.h"
#include "sync.h"

#include <atomic>
#include <string.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

// ============================================================
// Platform sockets
// ============================================================
#ifdef IS_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define INVALID_SOCK INVALID_SOCKET
#define close_socket closesocket
static int socket_error() { return WSAGetLastError(); }

static std::atomic<int> g_winsock_state{0};

static bool _winsock_init(char *err, size_t errlen) {
  int expected = 0;
  if (g_winsock_state.compare_exchange_strong(expected, 1)) {
    WSADATA wsa_data;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (rc != 0) {
      g_winsock_state.store(-1);
      if (err && errlen > 0) {
        snprintf(err, errlen, "WSAStartup failed: %d", rc);
      }
      return false;
    }
    g_winsock_state.store(2);
    return true;
  }

  int state = g_winsock_state.load();
  if (state == 2) return true;
  if (err && errlen > 0) {
    snprintf(err, errlen, "WSAStartup failed (previous error)");
  }
  return false;
}
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCK (-1)
#define close_socket close
static int socket_error() { return errno; }
#endif

// ============================================================
// TLS via SChannel (Windows native)
// ============================================================
#ifdef IS_WIN32
#define SECURITY_WIN32
#include <security.h>
#include <schannel.h>
#include <wincrypt.h>
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")

// Global credentials for client-side TLS
static CredHandle g_cred_handle = {{0}};
static bool g_cred_initialized = false;

static bool _tls_init(char *err, size_t errlen) {
  if (g_cred_initialized) return true;

  fprintf(stderr, "[TLS] Initializing SChannel credentials...\n");

  SCHANNEL_CRED schannel_cred = {};
  schannel_cred.dwVersion = SCHANNEL_CRED_VERSION;
  // Use 0 to let SChannel auto-negotiate the best available protocol (TLS 1.2, 1.3, etc.)
  // This avoids issues with SP_PROT_TLS1_3_CLIENT on older Windows versions
  schannel_cred.grbitEnabledProtocols = 0;
  // Use manual validation to be more permissive with certificates
  schannel_cred.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION | SCH_USE_STRONG_CRYPTO;

  TimeStamp ts = {};
  SECURITY_STATUS status = AcquireCredentialsHandleA(
      nullptr,
      UNISP_NAME_A,
      SECPKG_CRED_OUTBOUND,
      nullptr,
      &schannel_cred,
      nullptr,
      nullptr,
      &g_cred_handle,
      &ts);

  fprintf(stderr, "[TLS] AcquireCredentialsHandleA returned: 0x%08lx\n", status);

  if (status != SEC_E_OK) {
    if (err && errlen > 0) {
      snprintf(err, errlen, "AcquireCredentialsHandle failed: 0x%lx", status);
    }
    return false;
  }

  g_cred_initialized = true;
  return true;
}

static void _tls_cleanup(void) {
  if (g_cred_initialized && g_cred_handle.dwLower != 0) {
    FreeCredentialsHandle(&g_cred_handle);
    memset(&g_cred_handle, 0, sizeof(g_cred_handle));
    g_cred_initialized = false;
  }
}

#elif !defined(IS_HTML5)
// ============================================================
// TLS via dlopen'd OpenSSL (Linux/macOS)
// ============================================================
#include <dlfcn.h>

// opaque types — we never dereference these
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

struct OpenSSLLib {
  void *handle_ssl;
  void *handle_crypto;
  bool loaded;

  int (*OPENSSL_init_ssl)(uint64_t opts, void *settings);
  SSL_METHOD *(*TLS_client_method)(void);
  SSL_CTX *(*SSL_CTX_new)(const SSL_METHOD *method);
  void (*SSL_CTX_free)(SSL_CTX *ctx);
  long (*SSL_CTX_ctrl)(SSL_CTX *ctx, int cmd, long larg, void *parg);
  SSL *(*SSL_new)(SSL_CTX *ctx);
  void (*SSL_free)(SSL *ssl);
  int (*SSL_set_fd)(SSL *ssl, int fd);
  int (*SSL_connect)(SSL *ssl);
  int (*SSL_read)(SSL *ssl, void *buf, int num);
  int (*SSL_write)(SSL *ssl, const void *buf, int num);
  int (*SSL_shutdown)(SSL *ssl);
  long (*SSL_ctrl)(SSL *ssl, int cmd, long larg, void *parg);
  int (*SSL_get_error)(const SSL *ssl, int ret);
};

static OpenSSLLib _ossl;

#define SSL_CTRL_SET_TLSEXT_HOSTNAME 55
#define TLSEXT_NAMETYPE_host_name 0

static bool _tls_init(char *err, size_t errlen) {
  if (_ossl.loaded) return true;

  const char *ssl_names[] = {
      "libssl.so.3", "libssl.so.1.1", "libssl.so",
#ifdef __APPLE__
      "libssl.3.dylib", "libssl.1.1.dylib", "libssl.dylib",
#endif
      nullptr};
  const char *crypto_names[] = {
      "libcrypto.so.3", "libcrypto.so.1.1", "libcrypto.so",
#ifdef __APPLE__
      "libcrypto.3.dylib", "libcrypto.1.1.dylib", "libcrypto.dylib",
#endif
      nullptr};

  for (int i = 0; crypto_names[i]; i++) {
    _ossl.handle_crypto = dlopen(crypto_names[i], RTLD_LAZY | RTLD_LOCAL);
    if (_ossl.handle_crypto) break;
  }
  if (!_ossl.handle_crypto) {
    if (err && errlen > 0) {
      snprintf(err, errlen, "TLS not available (libcrypto not found)");
    }
    return false;
  }

  for (int i = 0; ssl_names[i]; i++) {
    _ossl.handle_ssl = dlopen(ssl_names[i], RTLD_LAZY | RTLD_LOCAL);
    if (_ossl.handle_ssl) break;
  }
  if (!_ossl.handle_ssl) {
    dlclose(_ossl.handle_crypto);
    _ossl.handle_crypto = nullptr;
    if (err && errlen > 0) {
      snprintf(err, errlen, "TLS not available (libssl not found)");
    }
    return false;
  }

  void *ssl = _ossl.handle_ssl;
#define OSSL_SYM(member, name) \
  *(void **)(&_ossl.member) = dlsym(ssl, name)

  OSSL_SYM(OPENSSL_init_ssl, "OPENSSL_init_ssl");
  OSSL_SYM(TLS_client_method, "TLS_client_method");
  OSSL_SYM(SSL_CTX_new, "SSL_CTX_new");
  OSSL_SYM(SSL_CTX_free, "SSL_CTX_free");
  OSSL_SYM(SSL_CTX_ctrl, "SSL_CTX_ctrl");
  OSSL_SYM(SSL_new, "SSL_new");
  OSSL_SYM(SSL_free, "SSL_free");
  OSSL_SYM(SSL_set_fd, "SSL_set_fd");
  OSSL_SYM(SSL_connect, "SSL_connect");
  OSSL_SYM(SSL_read, "SSL_read");
  OSSL_SYM(SSL_write, "SSL_write");
  OSSL_SYM(SSL_shutdown, "SSL_shutdown");
  OSSL_SYM(SSL_ctrl, "SSL_ctrl");
  OSSL_SYM(SSL_get_error, "SSL_get_error");
#undef OSSL_SYM

  if (!_ossl.SSL_CTX_new || !_ossl.TLS_client_method || !_ossl.SSL_new ||
      !_ossl.SSL_connect || !_ossl.SSL_read || !_ossl.SSL_write) {
    dlclose(_ossl.handle_ssl);
    dlclose(_ossl.handle_crypto);
    _ossl.handle_ssl = nullptr;
    _ossl.handle_crypto = nullptr;
    if (err && errlen > 0) {
      snprintf(err, errlen, "TLS not available (missing OpenSSL symbols)");
    }
    return false;
  }

  if (_ossl.OPENSSL_init_ssl) {
    _ossl.OPENSSL_init_ssl(0, nullptr);
  }

  _ossl.loaded = true;
  return true;
}

static void _tls_cleanup(void) {
  if (_ossl.handle_ssl) dlclose(_ossl.handle_ssl);
  if (_ossl.handle_crypto) dlclose(_ossl.handle_crypto);
  memset(&_ossl, 0, sizeof(_ossl));
}

#else // IS_HTML5
static bool _tls_init(char *err, size_t errlen) {
  (void)err; (void)errlen;
  return false;
}
static void _tls_cleanup(void) {}
#endif

// ============================================================
// URL parser
// ============================================================
struct ParsedUrl {
  bool https;
  char host[256];
  char port[8];
  char path[2048];
};

static bool _url_parse(const char *url, ParsedUrl *out) {
  memset(out, 0, sizeof(*out));

  if (strncmp(url, "https://", 8) == 0) {
    out->https = true;
    url += 8;
  } else if (strncmp(url, "http://", 7) == 0) {
    out->https = false;
    url += 7;
  } else {
    return false;
  }

  // host[:port]/path
  const char *slash = strchr(url, '/');
  const char *colon = strchr(url, ':');

  // colon must be before slash to be a port separator
  if (colon && slash && colon > slash) colon = nullptr;
  if (colon && !slash) {
    // host:port with no path
  }

  if (colon) {
    size_t hlen = (size_t)(colon - url);
    if (hlen >= sizeof(out->host)) return false;
    memcpy(out->host, url, hlen);
    out->host[hlen] = 0;

    const char *port_start = colon + 1;
    const char *port_end = slash ? slash : port_start + strlen(port_start);
    size_t plen = (size_t)(port_end - port_start);
    if (plen >= sizeof(out->port)) return false;
    memcpy(out->port, port_start, plen);
    out->port[plen] = 0;
  } else {
    const char *host_end = slash ? slash : url + strlen(url);
    size_t hlen = (size_t)(host_end - url);
    if (hlen >= sizeof(out->host)) return false;
    memcpy(out->host, url, hlen);
    out->host[hlen] = 0;
    snprintf(out->port, sizeof(out->port), "%d", out->https ? 443 : 80);
  }

  if (slash) {
    snprintf(out->path, sizeof(out->path), "%s", slash);
  } else {
    snprintf(out->path, sizeof(out->path), "/");
  }

  return true;
}

// ============================================================
// Socket + TLS connection wrapper
// ============================================================
struct Connection {
  socket_t sock;
#ifdef IS_WIN32
  CtxtHandle ctx_handle;    // SChannel context
  SecBuffer sec_buffers[4]; // for SChannel I/O
  char *read_buffer;        // buffered encrypted data from network
  u64 read_buffer_len;
  u64 read_buffer_cap;
  char *plain_buffer;       // buffered decrypted plaintext
  u64 plain_len;
  u64 plain_offset;
#elif !defined(IS_HTML5)
  SSL *ssl;
  SSL_CTX *ssl_ctx;
#endif
  bool is_tls;
};

// Forward declaration for SChannel handshake (defined later in Windows section)
#ifdef IS_WIN32
static bool _schannel_handshake(Connection *conn, const char *hostname,
                                 char *err, size_t errlen);
#endif

static bool _conn_connect(Connection *conn, ParsedUrl *url, char *err,
                           size_t errlen) {
  memset(conn, 0, sizeof(*conn));
  conn->sock = INVALID_SOCK;
  conn->is_tls = url->https;

  if (err && errlen > 0) {
    err[0] = 0;
  }

#ifdef IS_WIN32
  if (!_winsock_init(err, errlen)) {
    return false;
  }
#endif

  struct addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *result = nullptr;
  int rc = getaddrinfo(url->host, url->port, &hints, &result);
  if (rc != 0 || !result) {
    if (err && errlen > 0) {
#ifdef IS_WIN32
      snprintf(err, errlen, "getaddrinfo(%s:%s) failed: %s", url->host,
               url->port, gai_strerrorA(rc));
#else
      snprintf(err, errlen, "getaddrinfo(%s:%s) failed: %s", url->host,
               url->port, gai_strerror(rc));
#endif
    }
    return false;
  }

  socket_t sock = INVALID_SOCK;
  int last_err = 0;
  for (struct addrinfo *rp = result; rp; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == INVALID_SOCK) {
      last_err = socket_error();
      continue;
    }
    if (connect(sock, rp->ai_addr, (int)rp->ai_addrlen) == 0) break;
    last_err = socket_error();
    close_socket(sock);
    sock = INVALID_SOCK;
  }
  freeaddrinfo(result);

  if (sock == INVALID_SOCK) {
    if (err && errlen > 0) {
      snprintf(err, errlen, "connect(%s:%s) failed: %d", url->host, url->port,
               last_err);
    }
    return false;
  }
  conn->sock = sock;

#ifdef IS_WIN32
  if (url->https) {
    if (!_schannel_handshake(conn, url->host, err, errlen)) {
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }
  }
#elif !defined(IS_HTML5)
  if (url->https) {
    if (!_tls_init(err, errlen)) {
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }

    conn->ssl_ctx = _ossl.SSL_CTX_new(_ossl.TLS_client_method());
    if (!conn->ssl_ctx) {
      if (err && errlen > 0) {
        snprintf(err, errlen, "TLS context creation failed");
      }
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }

    conn->ssl = _ossl.SSL_new(conn->ssl_ctx);
    if (!conn->ssl) {
      if (err && errlen > 0) {
        snprintf(err, errlen, "TLS session creation failed");
      }
      _ossl.SSL_CTX_free(conn->ssl_ctx);
      conn->ssl_ctx = nullptr;
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }

    _ossl.SSL_set_fd(conn->ssl, (int)sock);

    // SNI: set hostname so virtual-hosted servers pick the right cert
    if (_ossl.SSL_ctrl) {
      _ossl.SSL_ctrl(conn->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME,
                     TLSEXT_NAMETYPE_host_name, (void *)url->host);
    }

    int ssl_ret = _ossl.SSL_connect(conn->ssl);
    if (ssl_ret <= 0) {
      if (err && errlen > 0 && _ossl.SSL_get_error) {
        int ssl_err = _ossl.SSL_get_error(conn->ssl, ssl_ret);
        snprintf(err, errlen, "TLS handshake failed: %d", ssl_err);
      }
      _ossl.SSL_free(conn->ssl);
      _ossl.SSL_CTX_free(conn->ssl_ctx);
      conn->ssl = nullptr;
      conn->ssl_ctx = nullptr;
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }
  }
#endif

  return true;
}

// ============================================================
// SChannel helpers (Windows TLS)
// ============================================================
#ifdef IS_WIN32

static bool _schannel_handshake(Connection *conn, const char *hostname,
                                 char *err, size_t errlen) {
  if (!_tls_init(err, errlen)) {
    return false;
  }

  fprintf(stderr, "[TLS] Starting handshake with %s\n", hostname);

  ULONG context_req =
      ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_INTEGRITY |
      ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_MANUAL_CRED_VALIDATION;

  SecBuffer in_buffers[2] = {};
  SecBufferDesc in_buffer_desc = {};
  SecBuffer out_buffers[1] = {};
  SecBufferDesc out_buffer_desc = {};
  
  char receive_buffer[16384];
  int receive_len = 0;
  
  in_buffer_desc.ulVersion = SECBUFFER_VERSION;
  in_buffer_desc.cBuffers = 2;
  in_buffer_desc.pBuffers = in_buffers;
  
  out_buffer_desc.ulVersion = SECBUFFER_VERSION;
  out_buffer_desc.cBuffers = 1;
  out_buffer_desc.pBuffers = out_buffers;
  
  ULONG context_attr = 0;
  TimeStamp ts = {};
  
  bool initial = true;
  
  // Perform the TLS handshake loop
  for (int i = 0; i < 100; i++) {  // max 100 iterations to prevent infinite loop
    fprintf(stderr, "[TLS] Iteration %d, initial=%d, receive_len=%d\n", i, initial, receive_len);
    // Setup output buffer for this iteration
    out_buffers[0].BufferType = SECBUFFER_TOKEN;
    out_buffers[0].cbBuffer = 0;
    out_buffers[0].pvBuffer = nullptr;
    
    // Setup input buffer if we've received data
    if (!initial) {
      in_buffers[0].BufferType = SECBUFFER_TOKEN;
      in_buffers[0].cbBuffer = (ULONG)receive_len;
      in_buffers[0].pvBuffer = receive_buffer;
      in_buffers[1].BufferType = SECBUFFER_EMPTY;
      in_buffers[1].cbBuffer = 0;
      in_buffers[1].pvBuffer = nullptr;
    }
    
    SECURITY_STATUS status = InitializeSecurityContextA(
        &g_cred_handle,
        initial ? nullptr : &conn->ctx_handle,
        (SEC_CHAR *)hostname,
        context_req,
        0,
        0,  // TargetDataRep: 0 for native byte order
        initial ? nullptr : &in_buffer_desc,
        0,
        initial ? &conn->ctx_handle : nullptr,
        &out_buffer_desc,
        &context_attr,
        &ts);
    
    fprintf(stderr, "[TLS] InitializeSecurityContextA returned: 0x%08lx, out_buffer_size=%lu\n", 
            status, out_buffers[0].cbBuffer);
    
    initial = false;
    
    if (status == SEC_E_OK) {
      fprintf(stderr, "[TLS] Handshake complete (SEC_E_OK)\n");
      // Handshake complete - send final token if any
      if (out_buffers[0].cbBuffer > 0) {
        int sent = (int)send(conn->sock, (const char *)out_buffers[0].pvBuffer,
                             (int)out_buffers[0].cbBuffer, 0);
        if (sent <= 0) {
          if (err && errlen > 0) {
            snprintf(err, errlen, "Failed to send final TLS handshake token");
          }
          if (out_buffers[0].pvBuffer) {
            FreeContextBuffer(out_buffers[0].pvBuffer);
          }
          return false;
        }
        if (out_buffers[0].pvBuffer) {
          FreeContextBuffer(out_buffers[0].pvBuffer);
        }
      }
      
      // Check for leftover application data (SECBUFFER_EXTRA)
      // The server might have sent app data along with handshake completion
      if (in_buffers[1].BufferType == SECBUFFER_EXTRA && in_buffers[1].cbBuffer > 0) {
        // Allocate read buffer and copy extra data
        conn->read_buffer_cap = 16384;
        conn->read_buffer = (char *)malloc(conn->read_buffer_cap);
        if (conn->read_buffer) {
          conn->read_buffer_len = in_buffers[1].cbBuffer;
          memcpy(conn->read_buffer, in_buffers[1].pvBuffer, in_buffers[1].cbBuffer);
        }
      }
      
      return true;
    } else if (status == SEC_I_CONTINUE_NEEDED || status == SEC_E_INCOMPLETE_MESSAGE) {
      fprintf(stderr, "[TLS] Continue needed (status=0x%08lx), sending token...\n", status);

      if (status == SEC_E_INCOMPLETE_MESSAGE) {
        // SEC_E_INCOMPLETE_MESSAGE: SChannel consumed NOTHING.
        // The entire receive_buffer is still valid — just append more data.
        fprintf(stderr, "[TLS] Incomplete message, keeping %d bytes and reading more\n", receive_len);
        int recvd = (int)recv(conn->sock, receive_buffer + receive_len,
                              sizeof(receive_buffer) - receive_len, 0);
        fprintf(stderr, "[TLS] Received %d bytes from server (appended to %d)\n", recvd, receive_len);
        if (recvd <= 0) {
          if (err && errlen > 0) {
            snprintf(err, errlen, "Failed to receive TLS handshake response");
          }
          return false;
        }
        receive_len += recvd;
        // Continue loop to retry with more data
      } else {
        // SEC_I_CONTINUE_NEEDED: Send token to server (if any)
        if (out_buffers[0].cbBuffer > 0) {
          int sent = (int)send(conn->sock, (const char *)out_buffers[0].pvBuffer,
                               (int)out_buffers[0].cbBuffer, 0);
          fprintf(stderr, "[TLS] Sent %d bytes to server\n", sent);
          if (sent <= 0) {
            if (err && errlen > 0) {
              snprintf(err, errlen, "Failed to send TLS handshake token");
            }
            if (out_buffers[0].pvBuffer) {
              FreeContextBuffer(out_buffers[0].pvBuffer);
            }
            return false;
          }
        }
        if (out_buffers[0].pvBuffer) {
          FreeContextBuffer(out_buffers[0].pvBuffer);
          out_buffers[0].pvBuffer = nullptr;
        }

        // Check for SECBUFFER_EXTRA - leftover data that wasn't consumed
        int extra_len = 0;
        for (int j = 0; j < 2; j++) {
          if (in_buffers[j].BufferType == SECBUFFER_EXTRA && in_buffers[j].cbBuffer > 0) {
            extra_len = (int)in_buffers[j].cbBuffer;
            fprintf(stderr, "[TLS] Found SECBUFFER_EXTRA: %d bytes\n", extra_len);
            // Move extra data to beginning of buffer
            memmove(receive_buffer, 
                    receive_buffer + (receive_len - extra_len),
                    extra_len);
            break;
          }
        }

        if (extra_len > 0) {
          // We have extra data, use it for next iteration without receiving
          receive_len = extra_len;
          fprintf(stderr, "[TLS] Using buffered EXTRA data for next iteration\n");
        } else {
          // Need fresh data from server
          int recvd = (int)recv(conn->sock, receive_buffer, 
                                sizeof(receive_buffer), 0);
          fprintf(stderr, "[TLS] Received %d bytes from server\n", recvd);
          if (recvd <= 0) {
            if (err && errlen > 0) {
              snprintf(err, errlen, "Failed to receive TLS handshake response");
            }
            return false;
          }
          receive_len = recvd;
        }
      }
      // Continue loop to process the received data
    } else {
      fprintf(stderr, "[TLS] Handshake failed with status: 0x%08lx\n", status);
      if (err && errlen > 0) {
        snprintf(err, errlen, "TLS handshake failed: 0x%lx", status);
      }
      if (out_buffers[0].pvBuffer) {
        FreeContextBuffer(out_buffers[0].pvBuffer);
      }
      return false;
    }
  }
  
  if (err && errlen > 0) {
    snprintf(err, errlen, "TLS handshake timeout (too many iterations)");
  }
  return false;
}

static int _schannel_send(Connection *conn, const void *data, int len) {
  if (!data || len <= 0) return 0;

  // Get security header/trailer sizes
  SecPkgContext_StreamSizes sizes = {};
  SECURITY_STATUS status =
      QueryContextAttributesA(&conn->ctx_handle, SECPKG_ATTR_STREAM_SIZES, &sizes);
  if (status != SEC_E_OK) {
    return -1;
  }

  DWORD total_len = sizes.cbHeader + len + sizes.cbTrailer;
  char *buffer = (char *)malloc(total_len);
  if (!buffer) return -1;

  memcpy(buffer + sizes.cbHeader, data, len);

  SecBuffer buffers[4];
  buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
  buffers[0].pvBuffer = buffer;
  buffers[0].cbBuffer = sizes.cbHeader;
  buffers[1].BufferType = SECBUFFER_DATA;
  buffers[1].pvBuffer = buffer + sizes.cbHeader;
  buffers[1].cbBuffer = len;
  buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
  buffers[2].pvBuffer = buffer + sizes.cbHeader + len;
  buffers[2].cbBuffer = sizes.cbTrailer;
  buffers[3].BufferType = SECBUFFER_EMPTY;
  buffers[3].pvBuffer = nullptr;
  buffers[3].cbBuffer = 0;

  SecBufferDesc buffer_desc;
  buffer_desc.ulVersion = SECBUFFER_VERSION;
  buffer_desc.cBuffers = 4;
  buffer_desc.pBuffers = buffers;

  status = EncryptMessage(&conn->ctx_handle, 0, &buffer_desc, 0);
  if (status != SEC_E_OK) {
    free(buffer);
    return -1;
  }

  int sent = (int)send(conn->sock, buffer, total_len, 0);
  free(buffer);
  return sent > 0 ? len : -1;  // Return requested length on success
}

static int _schannel_recv(Connection *conn, void *buf, int len) {
  if (!buf || len <= 0) return -1;

  // 1. Return any buffered plaintext from a previous decryption
  if (conn->plain_buffer && conn->plain_offset < conn->plain_len) {
    u64 avail = conn->plain_len - conn->plain_offset;
    int to_copy = (int)(avail < (u64)len ? avail : (u64)len);
    memcpy(buf, conn->plain_buffer + conn->plain_offset, to_copy);
    conn->plain_offset += to_copy;
    // Free plaintext buffer when fully consumed
    if (conn->plain_offset >= conn->plain_len) {
      free(conn->plain_buffer);
      conn->plain_buffer = nullptr;
      conn->plain_len = 0;
      conn->plain_offset = 0;
    }
    return to_copy;
  }

  // 2. Ensure we have an encrypted read buffer
  if (!conn->read_buffer) {
    conn->read_buffer_cap = 16384;
    conn->read_buffer = (char *)malloc(conn->read_buffer_cap);
    if (!conn->read_buffer) return -1;
    conn->read_buffer_len = 0;
  }

  while (true) {
    // If encrypted buffer is empty, read from network
    if (conn->read_buffer_len == 0) {
      int recvd = (int)recv(conn->sock, conn->read_buffer, (int)conn->read_buffer_cap, 0);
      if (recvd <= 0) {
        return recvd;  // Connection closed or error
      }
      conn->read_buffer_len = recvd;
    }

    // Try to decrypt
    SecBuffer buffers[4];
    buffers[0].BufferType = SECBUFFER_DATA;
    buffers[0].pvBuffer = conn->read_buffer;
    buffers[0].cbBuffer = (ULONG)conn->read_buffer_len;
    buffers[1].BufferType = SECBUFFER_EMPTY;
    buffers[2].BufferType = SECBUFFER_EMPTY;
    buffers[3].BufferType = SECBUFFER_EMPTY;

    SecBufferDesc buffer_desc;
    buffer_desc.ulVersion = SECBUFFER_VERSION;
    buffer_desc.cBuffers = 4;
    buffer_desc.pBuffers = buffers;

    SECURITY_STATUS status = DecryptMessage(&conn->ctx_handle, &buffer_desc, 0, nullptr);

    if (status == SEC_E_OK) {
      // Find the plaintext data buffer (DecryptMessage decrypts in-place)
      char *plain_data = nullptr;
      ULONG plain_size = 0;
      for (int i = 0; i < 4; i++) {
        if (buffers[i].BufferType == SECBUFFER_DATA && buffers[i].cbBuffer > 0) {
          plain_data = (char *)buffers[i].pvBuffer;
          plain_size = buffers[i].cbBuffer;
          break;
        }
      }

      if (!plain_data || plain_size == 0) {
        // Preserve SECBUFFER_EXTRA before continuing
        for (int i = 0; i < 4; i++) {
          if (buffers[i].BufferType == SECBUFFER_EXTRA && buffers[i].cbBuffer > 0) {
            memmove(conn->read_buffer,
                    conn->read_buffer + (conn->read_buffer_len - buffers[i].cbBuffer),
                    buffers[i].cbBuffer);
            conn->read_buffer_len = buffers[i].cbBuffer;
            goto next_iteration;
          }
        }
        conn->read_buffer_len = 0;
        next_iteration:
        continue;  // Empty record, try again
      }

      // Copy plaintext BEFORE memmove (both point into read_buffer)
      int to_copy = plain_size < (ULONG)len ? (int)plain_size : len;
      memcpy(buf, plain_data, to_copy);

      // Buffer remaining plaintext for subsequent reads
      if ((ULONG)to_copy < plain_size) {
        u64 remaining = plain_size - to_copy;
        conn->plain_buffer = (char *)malloc(remaining);
        if (conn->plain_buffer) {
          memcpy(conn->plain_buffer, plain_data + to_copy, remaining);
          conn->plain_len = remaining;
          conn->plain_offset = 0;
        }
      }

      // NOW safe to memmove leftover encrypted data
      bool has_extra = false;
      for (int i = 0; i < 4; i++) {
        if (buffers[i].BufferType == SECBUFFER_EXTRA && buffers[i].cbBuffer > 0) {
          memmove(conn->read_buffer,
                  conn->read_buffer + (conn->read_buffer_len - buffers[i].cbBuffer),
                  buffers[i].cbBuffer);
          conn->read_buffer_len = buffers[i].cbBuffer;
          has_extra = true;
          break;
        }
      }
      if (!has_extra) {
        conn->read_buffer_len = 0;
      }

      return to_copy;

    } else if (status == SEC_E_INCOMPLETE_MESSAGE) {
      // Need more data to form a complete TLS record
      if (conn->read_buffer_len >= conn->read_buffer_cap) {
        char *new_buffer = (char *)realloc(conn->read_buffer, conn->read_buffer_cap * 2);
        if (!new_buffer) return -1;
        conn->read_buffer = new_buffer;
        conn->read_buffer_cap *= 2;
      }

      int recvd = (int)recv(conn->sock, conn->read_buffer + conn->read_buffer_len,
                            (int)(conn->read_buffer_cap - conn->read_buffer_len), 0);
      if (recvd <= 0) {
        return recvd;  // Connection closed or error
      }
      conn->read_buffer_len += recvd;
      // Loop to retry decryption

    } else if (status == (SECURITY_STATUS)0x00090317) {
      // SEC_I_CONTEXT_EXPIRED: server sent TLS close_notify
      conn->read_buffer_len = 0;
      return 0;  // Graceful EOF

    } else {
      return -1;  // Decryption error
    }
  }

  return -1;
}

#endif  // IS_WIN32

// ============================================================
// Connection I/O wrapper
// ============================================================
static int _conn_write(Connection *conn, const void *buf, int len) {
#ifdef IS_WIN32
  if (conn->is_tls) {
    return _schannel_send(conn, buf, len);
  }
#elif !defined(IS_HTML5)
  if (conn->is_tls && conn->ssl) {
    return _ossl.SSL_write(conn->ssl, buf, len);
  }
#endif
  return (int)send(conn->sock, (const char *)buf, len, 0);
}

static int _conn_read(Connection *conn, void *buf, int len) {
#ifdef IS_WIN32
  if (conn->is_tls) {
    return _schannel_recv(conn, buf, len);
  }
#elif !defined(IS_HTML5)
  if (conn->is_tls && conn->ssl) {
    return _ossl.SSL_read(conn->ssl, buf, len);
  }
#endif
  return (int)recv(conn->sock, (char *)buf, len, 0);
}

static void _conn_close(Connection *conn) {
#ifdef IS_WIN32
  if (conn->is_tls && conn->ctx_handle.dwLower != 0) {
    DeleteSecurityContext(&conn->ctx_handle);
    memset(&conn->ctx_handle, 0, sizeof(conn->ctx_handle));
  }
  if (conn->read_buffer) {
    free(conn->read_buffer);
    conn->read_buffer = nullptr;
    conn->read_buffer_len = 0;
    conn->read_buffer_cap = 0;
  }
  if (conn->plain_buffer) {
    free(conn->plain_buffer);
    conn->plain_buffer = nullptr;
    conn->plain_len = 0;
    conn->plain_offset = 0;
  }
#elif !defined(IS_HTML5)
  if (conn->ssl) {
    _ossl.SSL_shutdown(conn->ssl);
    _ossl.SSL_free(conn->ssl);
    conn->ssl = nullptr;
  }
  if (conn->ssl_ctx) {
    _ossl.SSL_CTX_free(conn->ssl_ctx);
    conn->ssl_ctx = nullptr;
  }
#endif
  if (conn->sock != INVALID_SOCK) {
    close_socket(conn->sock);
    conn->sock = INVALID_SOCK;
  }
}


// ============================================================
// Dynamic byte buffer (arena-free, uses malloc/free for thread)
// ============================================================
struct ByteBuf {
  char *data;
  u64 len;
  u64 cap;

  void init() { data = nullptr; len = 0; cap = 0; }
  void trash() { ::free(data); data = nullptr; len = 0; cap = 0; }

  void ensure(u64 extra) {
    u64 need = len + extra;
    if (need <= cap) return;
    u64 new_cap = cap > 0 ? cap * 2 : 4096;
    while (new_cap < need) new_cap *= 2;
    data = (char *)::realloc(data, new_cap);
    cap = new_cap;
  }

  void append(const char *src, u64 n) {
    ensure(n);
    memcpy(data + len, src, n);
    len += n;
  }

  void append_str(const char *s) { append(s, strlen(s)); }
  void append_char(char c) { ensure(1); data[len++] = c; }

  // null-terminate without changing len
  void null_terminate() {
    ensure(1);
    data[len] = '\0';
  }
};

// ============================================================
// HTTP request/response
// ============================================================
struct HttpHeader {
  char *name;
  char *value;
};

struct HttpRequest {
  // -- input (set before starting thread) --
  char *url;
  char *method;
  char *body;
  u64 body_len;
  HttpHeader *headers;
  int header_count;
  float timeout_secs;

  // -- output (set by worker thread) --
  ByteBuf response_body;
  int status_code;
  ByteBuf response_headers_raw;

  char *output_path; // optional: stream response body to file
  bool output_override; // overwrite existing file (default: false = resume)

  char error[512];

  // -- progress tracking (thread-safe) --
  std::atomic<u64> bytes_uploaded;
  std::atomic<u64> bytes_downloaded;
  std::atomic<i64> content_length;  // -1 if unknown

  // -- synchronization --
  std::atomic<int> state; // 0=running, 1=done, 2=error
  Thread thread;
};

// send exactly `len` bytes
static bool _send_all(Connection *conn, const char *data, int len, HttpRequest *req = nullptr) {
  int sent = 0;
  while (sent < len) {
    int n = _conn_write(conn, data + sent, len - sent);
    if (n <= 0) return false;
    sent += n;
    if (req) {
      req->bytes_uploaded.fetch_add(n, std::memory_order_relaxed);
    }
  }
  return true;
}

// read a line ending in \r\n, stores without the \r\n
static bool _read_line(Connection *conn, ByteBuf *line) {
  line->len = 0;
  while (true) {
    char c;
    int n = _conn_read(conn, &c, 1);
    if (n <= 0) return false;
    if (c == '\r') {
      n = _conn_read(conn, &c, 1);
      if (n <= 0) return false;
      if (c == '\n') break;
      line->append_char('\r');
      line->append_char(c);
    } else {
      line->append_char(c);
    }
  }
  line->null_terminate();
  return true;
}

// read exactly `count` bytes
static bool _read_exact(Connection *conn, ByteBuf *buf, u64 count, HttpRequest *req = nullptr) {
  buf->ensure(count);
  u64 total = 0;
  while (total < count) {
    int chunk = (int)((count - total) > 65536 ? 65536 : (count - total));
    int n = _conn_read(conn, buf->data + buf->len, chunk);
    if (n <= 0) return false;
    buf->len += n;
    total += n;
    if (req) {
      req->bytes_downloaded.fetch_add(n, std::memory_order_relaxed);
    }
  }
  return true;
}

// parse hex string to u64
static u64 _hex_to_u64(const char *s) {
  u64 val = 0;
  while (*s) {
    char c = *s++;
    val <<= 4;
    if (c >= '0' && c <= '9')
      val |= (u64)(c - '0');
    else if (c >= 'a' && c <= 'f')
      val |= (u64)(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
      val |= (u64)(c - 'A' + 10);
    else
      break;
  }
  return val;
}

static void _http_worker(void *udata) {
  HttpRequest *req = (HttpRequest *)udata;
  req->response_body.init();
  req->response_headers_raw.init();
  req->status_code = 0;
  req->error[0] = 0;
  req->bytes_uploaded.store(0, std::memory_order_relaxed);
  req->bytes_downloaded.store(0, std::memory_order_relaxed);
  req->content_length.store(-1, std::memory_order_relaxed);

  FILE *out_file = nullptr;

  // Check for resume: if output file exists and override is not set, get existing size
  i64 resume_offset = 0;
  if (req->output_path && !req->output_override) {
    FILE *f = fopen(req->output_path, "rb");
    if (f) {
#ifdef IS_WIN32
      _fseeki64(f, 0, SEEK_END);
      resume_offset = _ftelli64(f);
#else
      fseeko(f, 0, SEEK_END);
      resume_offset = (i64)ftello(f);
#endif
      fclose(f);
      if (resume_offset > 0) {
        fprintf(stderr, "[HTTP] Resume: existing file %s is %lld bytes\n",
                req->output_path, (long long)resume_offset);
      }
    }
  }

  // current URL (may change on redirect)
  char *current_url = _strdup(req->url);
  if (!current_url) {
    snprintf(req->error, sizeof(req->error), "out of memory");
    req->state.store(2, std::memory_order_release);
    return;
  }

  static const int MAX_REDIRECTS = 10;

  for (int redirect_count = 0; redirect_count <= MAX_REDIRECTS; redirect_count++) {

  ParsedUrl url;
  if (!_url_parse(current_url, &url)) {
    snprintf(req->error, sizeof(req->error), "invalid URL: %s", current_url);
    ::free(current_url);
    req->state.store(2, std::memory_order_release);
    return;
  }

#if !defined(IS_HTML5)
  if (url.https && !_tls_init(req->error, sizeof(req->error))) {
    ::free(current_url);
    req->state.store(2, std::memory_order_release);
    return;
  }
#else
  if (url.https) {
    snprintf(req->error, sizeof(req->error),
             "HTTPS not available on this platform");
    ::free(current_url);
    req->state.store(2, std::memory_order_release);
    return;
  }
#endif

  Connection conn;
  if (!_conn_connect(&conn, &url, req->error, sizeof(req->error))) {
    if (req->error[0] == 0) {
      snprintf(req->error, sizeof(req->error), "connection to %s:%s failed",
               url.host, url.port);
    }
    ::free(current_url);
    req->state.store(2, std::memory_order_release);
    return;
  }

  // -- build request --
  ByteBuf sendbuf;
  sendbuf.init();
  {
    // request line
    sendbuf.append_str(req->method);
    sendbuf.append_char(' ');
    sendbuf.append_str(url.path);
    sendbuf.append_str(" HTTP/1.1\r\n");

    // host header
    sendbuf.append_str("Host: ");
    sendbuf.append_str(url.host);
    sendbuf.append_str("\r\n");

    // user-agent
    sendbuf.append_str("User-Agent: Spry/1.0\r\n");

    // connection: close (we don't reuse connections)
    sendbuf.append_str("Connection: close\r\n");

    // custom headers
    for (int i = 0; i < req->header_count; i++) {
      sendbuf.append_str(req->headers[i].name);
      sendbuf.append_str(": ");
      sendbuf.append_str(req->headers[i].value);
      sendbuf.append_str("\r\n");
    }

    // Range header for resume
    if (resume_offset > 0) {
      char range[128];
      snprintf(range, sizeof(range), "Range: bytes=%lld-\r\n", (long long)resume_offset);
      sendbuf.append_str(range);
    }

    // body
    if (req->body && req->body_len > 0) {
      char cl[64];
      snprintf(cl, sizeof(cl), "Content-Length: %llu\r\n",
               (unsigned long long)req->body_len);
      sendbuf.append_str(cl);
    }

    sendbuf.append_str("\r\n");

    // body payload
    if (req->body && req->body_len > 0) {
      sendbuf.append(req->body, req->body_len);
    }
  }

  if (!_send_all(&conn, sendbuf.data, (int)sendbuf.len, req)) {
    snprintf(req->error, sizeof(req->error), "failed to send request");
    sendbuf.trash();
    _conn_close(&conn);
    if (out_file) fclose(out_file);
    ::free(current_url);
    req->state.store(2, std::memory_order_release);
    return;
  }
  sendbuf.trash();

  // -- read response --
  ByteBuf line;
  line.init();

  // status line: HTTP/1.1 200 OK
  if (!_read_line(&conn, &line)) {
    snprintf(req->error, sizeof(req->error), "failed to read status line");
    line.trash();
    _conn_close(&conn);
    if (out_file) fclose(out_file);
    ::free(current_url);
    req->state.store(2, std::memory_order_release);
    return;
  }

  // parse status code
  {
    const char *p = line.data;
    // skip "HTTP/x.x "
    while (*p && *p != ' ') p++;
    while (*p == ' ') p++;
    req->status_code = (int)strtol(p, nullptr, 10);
  }

  // read headers
  i64 content_length = -1;
  bool chunked = false;
  char location[2048] = {};

  while (true) {
    if (!_read_line(&conn, &line)) {
      snprintf(req->error, sizeof(req->error), "failed to read headers");
      line.trash();
      _conn_close(&conn);
      if (out_file) fclose(out_file);
      ::free(current_url);
      req->state.store(2, std::memory_order_release);
      return;
    }
    if (line.len == 0) break; // end of headers

    // store raw header line
    req->response_headers_raw.append(line.data, line.len);
    req->response_headers_raw.append_char('\n');

    // check for content-length (case-insensitive)
    const char *colon = strchr(line.data, ':');
    if (colon) {
      size_t name_len = (size_t)(colon - line.data);
      const char *val = colon + 1;
      while (*val == ' ') val++;

      // case-insensitive compare
      auto ci_eq = [](const char *a, const char *b, size_t n) -> bool {
        for (size_t i = 0; i < n; i++) {
          char ca = a[i] >= 'A' && a[i] <= 'Z' ? a[i] + 32 : a[i];
          char cb = b[i] >= 'A' && b[i] <= 'Z' ? b[i] + 32 : b[i];
          if (ca != cb) return false;
        }
        return true;
      };

      if (name_len == 14 && ci_eq(line.data, "content-length", 14)) {
        content_length = (i64)strtoll(val, nullptr, 10);
        req->content_length.store(content_length, std::memory_order_relaxed);
      }
      if (name_len == 17 && ci_eq(line.data, "transfer-encoding", 17)) {
        // check if "chunked" appears in the value
        const char *ch = strstr(val, "chunked");
        if (!ch) ch = strstr(val, "Chunked");
        if (ch) chunked = true;
      }
      if (name_len == 8 && ci_eq(line.data, "location", 8)) {
        snprintf(location, sizeof(location), "%s", val);
      }
    }
  }
  req->response_headers_raw.null_terminate();

  // Handle redirects (301, 302, 303, 307, 308)
  if ((req->status_code >= 301 && req->status_code <= 303) ||
      req->status_code == 307 || req->status_code == 308) {
    if (location[0] != 0) {
      // Drain any body so the connection closes cleanly
      line.trash();
      _conn_close(&conn);

      // Resolve relative URLs
      char *new_url = nullptr;
      if (location[0] == '/') {
        // Relative URL - combine with current scheme + host
        size_t needed = strlen(url.https ? "https://" : "http://") +
                        strlen(url.host) + strlen(location) + 8;
        new_url = (char *)::malloc(needed);
        if (new_url) {
          snprintf(new_url, needed, "%s%s%s%s%s",
                   url.https ? "https://" : "http://",
                   url.host,
                   (url.https && strcmp(url.port, "443") != 0) ||
                   (!url.https && strcmp(url.port, "80") != 0) ? ":" : "",
                   (url.https && strcmp(url.port, "443") != 0) ||
                   (!url.https && strcmp(url.port, "80") != 0) ? url.port : "",
                   location);
        }
      } else {
        new_url = _strdup(location);
      }

      if (!new_url) {
        snprintf(req->error, sizeof(req->error), "out of memory on redirect");
        ::free(current_url);
        req->state.store(2, std::memory_order_release);
        return;
      }

      fprintf(stderr, "[HTTP] Redirect %d: %s -> %s\n", req->status_code, current_url, new_url);
      ::free(current_url);
      current_url = new_url;

      // Reset for next request
      req->response_headers_raw.trash();
      req->response_headers_raw.init();
      req->response_body.trash();
      req->response_body.init();
      req->content_length.store(-1, std::memory_order_relaxed);

      // For 303 redirects, switch to GET
      if (req->status_code == 303) {
        ::free(req->method);
        req->method = _strdup("GET");
      }

      continue; // retry with new URL
    }
  }

  // Open output file only after redirects are resolved
  if (!out_file && req->output_path) {
    if (resume_offset > 0 && req->status_code == 206) {
      // Server supports resume (206 Partial Content) — append
      out_file = fopen(req->output_path, "ab");
      if (out_file) {
        // Adjust progress: we already have resume_offset bytes
        req->bytes_downloaded.store((u64)resume_offset, std::memory_order_relaxed);
        // Adjust total: content_length is remaining bytes, total = remaining + already downloaded
        if (content_length >= 0) {
          i64 total = content_length + resume_offset;
          req->content_length.store(total, std::memory_order_relaxed);
        }
        fprintf(stderr, "[HTTP] Resuming at offset %lld (status 206)\n", (long long)resume_offset);
      }
    } else {
      // Fresh download (200), or override, or server doesn't support Range
      if (resume_offset > 0 && req->status_code == 200) {
        fprintf(stderr, "[HTTP] Server returned 200 (no Range support), restarting download\n");
      }
      out_file = fopen(req->output_path, "wb");
      resume_offset = 0; // not resuming
    }
    if (!out_file) {
      snprintf(req->error, sizeof(req->error), "failed to open output file: %s", req->output_path);
      line.trash();
      _conn_close(&conn);
      req->state.store(2, std::memory_order_release);
      ::free(current_url);
      return;
    }
  }

  // read body
  if (chunked) {
    // chunked transfer encoding
    while (true) {
      if (!_read_line(&conn, &line)) break;
      u64 chunk_size = _hex_to_u64(line.data);
      if (chunk_size == 0) {
        // read trailing \r\n
        _read_line(&conn, &line);
        break;
      }
      if (out_file) {
        u64 remaining = chunk_size;
        char buf[4096];
        while (remaining > 0) {
          int chunk = (int)(remaining > sizeof(buf) ? sizeof(buf) : remaining);
          int n = _conn_read(&conn, buf, chunk);
          if (n <= 0) {
            snprintf(req->error, sizeof(req->error), "failed to read chunked body");
            line.trash();
            _conn_close(&conn);
            fclose(out_file);
            ::free(current_url);
            req->state.store(2, std::memory_order_release);
            return;
          }
          if ((int)fwrite(buf, 1, n, out_file) != n) {
            snprintf(req->error, sizeof(req->error), "failed to write output file");
            line.trash();
            _conn_close(&conn);
            fclose(out_file);
            ::free(current_url);
            req->state.store(2, std::memory_order_release);
            return;
          }
          req->bytes_downloaded.fetch_add(n, std::memory_order_relaxed);
          remaining -= n;
        }
      } else if (!_read_exact(&conn, &req->response_body, chunk_size, req)) {
        snprintf(req->error, sizeof(req->error),
                 "failed to read chunked body");
        line.trash();
        _conn_close(&conn);
        if (out_file) fclose(out_file);
        ::free(current_url);
        req->state.store(2, std::memory_order_release);
        return;
      }
      // read chunk trailing \r\n
      _read_line(&conn, &line);
    }
  } else if (content_length >= 0) {
    if (out_file) {
      u64 remaining = (u64)content_length;
      char buf[4096];
      while (remaining > 0) {
        int chunk = (int)(remaining > sizeof(buf) ? sizeof(buf) : remaining);
        int n = _conn_read(&conn, buf, chunk);
        if (n <= 0) {
          snprintf(req->error, sizeof(req->error), "failed to read body");
          line.trash();
          _conn_close(&conn);
          fclose(out_file);
          ::free(current_url);
          req->state.store(2, std::memory_order_release);
          return;
        }
        if ((int)fwrite(buf, 1, n, out_file) != n) {
          snprintf(req->error, sizeof(req->error), "failed to write output file");
          line.trash();
          _conn_close(&conn);
          fclose(out_file);
          ::free(current_url);
          req->state.store(2, std::memory_order_release);
          return;
        }
        req->bytes_downloaded.fetch_add(n, std::memory_order_relaxed);
        remaining -= n;
      }
    } else if (!_read_exact(&conn, &req->response_body, (u64)content_length, req)) {
      snprintf(req->error, sizeof(req->error), "failed to read body");
      line.trash();
      _conn_close(&conn);
      if (out_file) fclose(out_file);
      ::free(current_url);
      req->state.store(2, std::memory_order_release);
      return;
    }
  } else {
    // read until connection closes
    char buf[4096];
    while (true) {
      int n = _conn_read(&conn, buf, sizeof(buf));
      if (n <= 0) break;
      if (out_file) {
        if ((int)fwrite(buf, 1, n, out_file) != n) {
          snprintf(req->error, sizeof(req->error), "failed to write output file");
          line.trash();
          _conn_close(&conn);
          fclose(out_file);
          ::free(current_url);
          req->state.store(2, std::memory_order_release);
          return;
        }
      } else {
        req->response_body.append(buf, n);
      }
      req->bytes_downloaded.fetch_add(n, std::memory_order_relaxed);
    }
  }
  if (!out_file) {
    req->response_body.null_terminate();
  }

  line.trash();
  _conn_close(&conn);
  if (out_file) fclose(out_file);
  ::free(current_url);
  req->state.store(1, std::memory_order_release);
  return; // success — break out of redirect loop

  } // end of redirect loop

  // If we get here, too many redirects
  snprintf(req->error, sizeof(req->error), "too many redirects (max %d)", MAX_REDIRECTS);
  ::free(current_url);
  req->state.store(2, std::memory_order_release);
}

// ============================================================
// Lua API
// ============================================================

// HttpRequest** userdata stored in Lua
#define HTTP_REQUEST_MT "mt_http_request"

static int mt_http_request_gc(lua_State *L) {
  HttpRequest **pptr = (HttpRequest **)luaL_checkudata(L, 1, HTTP_REQUEST_MT);
  HttpRequest *req = *pptr;
  if (req) {
    // wait for thread to finish
    if (req->state.load(std::memory_order_acquire) == 0) {
      req->thread.join();
    } else {
      req->thread.join();
    }

    // free everything
    ::free(req->url);
    ::free(req->method);
    ::free(req->body);
    ::free(req->output_path);
    for (int i = 0; i < req->header_count; i++) {
      ::free(req->headers[i].name);
      ::free(req->headers[i].value);
    }
    ::free(req->headers);
    req->response_body.trash();
    req->response_headers_raw.trash();
    ::free(req);
    *pptr = nullptr;
  }
  return 0;
}

// req:done() -> bool
static int mt_http_request_done(lua_State *L) {
  HttpRequest **pptr = (HttpRequest **)luaL_checkudata(L, 1, HTTP_REQUEST_MT);
  HttpRequest *req = *pptr;
  if (!req) {
    lua_pushboolean(L, true);
    return 1;
  }
  int st = req->state.load(std::memory_order_acquire);
  lua_pushboolean(L, st != 0);
  return 1;
}

// push response headers as a Lua table
static void _push_headers_table(lua_State *L, const char *raw) {
  lua_newtable(L);
  if (!raw) return;

  const char *p = raw;
  while (*p) {
    const char *line_end = strchr(p, '\n');
    if (!line_end) line_end = p + strlen(p);

    const char *colon = strchr(p, ':');
    if (colon && colon < line_end) {
      size_t name_len = (size_t)(colon - p);
      const char *val = colon + 1;
      while (*val == ' ' && val < line_end) val++;
      size_t val_len = (size_t)(line_end - val);

      // lowercase header name for consistency
      char name_buf[256];
      size_t copy_len = name_len < sizeof(name_buf) - 1 ? name_len
                                                          : sizeof(name_buf) - 1;
      for (size_t i = 0; i < copy_len; i++) {
        char c = p[i];
        name_buf[i] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
      }
      name_buf[copy_len] = 0;

      lua_pushlstring(L, name_buf, copy_len);
      lua_pushlstring(L, val, val_len);
      lua_settable(L, -3);
    }

    if (*line_end == '\n')
      p = line_end + 1;
    else
      break;
  }
}

// req:result() -> body, status, headers, err
static int mt_http_request_result(lua_State *L) {
  HttpRequest **pptr = (HttpRequest **)luaL_checkudata(L, 1, HTTP_REQUEST_MT);
  HttpRequest *req = *pptr;
  if (!req) {
    lua_pushnil(L);
    lua_pushinteger(L, 0);
    lua_newtable(L);
    lua_pushstring(L, "request already consumed");
    return 4;
  }

  int st = req->state.load(std::memory_order_acquire);
  if (st == 0) {
    // still running
    lua_pushnil(L);
    lua_pushinteger(L, 0);
    lua_newtable(L);
    lua_pushstring(L, "request still in progress");
    return 4;
  }

  // join thread
  req->thread.join();

  if (st == 2) {
    // error
    lua_pushnil(L);
    lua_pushinteger(L, 0);
    lua_newtable(L);
    lua_pushstring(L, req->error);
    return 4;
  }

  // success
  if (req->output_path) {
    lua_pushnil(L);
  } else {
    lua_pushlstring(L, req->response_body.data, req->response_body.len);
  }
  lua_pushinteger(L, req->status_code);
  _push_headers_table(L, req->response_headers_raw.data);
  lua_pushnil(L); // no error
  return 4;
}

// req:progress() -> {uploaded, downloaded, total}
static int mt_http_request_progress(lua_State *L) {
  HttpRequest **pptr = (HttpRequest **)luaL_checkudata(L, 1, HTTP_REQUEST_MT);
  HttpRequest *req = *pptr;
  if (!req) {
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "uploaded");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "downloaded");
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "total");
    return 1;
  }

  u64 uploaded = req->bytes_uploaded.load(std::memory_order_relaxed);
  u64 downloaded = req->bytes_downloaded.load(std::memory_order_relaxed);
  i64 total = req->content_length.load(std::memory_order_relaxed);

  lua_newtable(L);
  lua_pushinteger(L, (lua_Integer)uploaded);
  lua_setfield(L, -2, "uploaded");
  lua_pushinteger(L, (lua_Integer)downloaded);
  lua_setfield(L, -2, "downloaded");
  lua_pushinteger(L, (lua_Integer)total);
  lua_setfield(L, -2, "total");
  return 1;
}

static int open_mt_http_request(lua_State *L) {
  luaL_Reg reg[] = {
      {"__gc", mt_http_request_gc},
      {"done", mt_http_request_done},
      {"result", mt_http_request_result},
      {"progress", mt_http_request_progress},
      {nullptr, nullptr},
  };
  luax_new_class(L, HTTP_REQUEST_MT, reg);
  return 0;
}

// dup a C string using malloc (safe for worker thread)
static char *_strdup_malloc(const char *s) {
  if (!s) return nullptr;
  size_t len = strlen(s) + 1;
  char *out = (char *)::malloc(len);
  memcpy(out, s, len);
  return out;
}

static char *_lstrdup_malloc(const char *s, size_t len) {
  char *out = (char *)::malloc(len + 1);
  memcpy(out, s, len);
  out[len] = 0;
  return out;
}

// spry.http._request(opts) -> HttpRequest userdata
//   opts = {
//     url     = string (required),
//     method  = string (default "GET"),
//     headers = { ["Key"] = "Value", ... } (optional),
//     body    = string (optional),
//     timeout = number (optional, seconds, default 30),
//     output  = string (optional file path to write response body),
//   }
static int spry_http_request(lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);

  // url (required)
  lua_getfield(L, 1, "url");
  const char *url = luaL_checkstring(L, -1);
  lua_pop(L, 1);

  // method
  lua_getfield(L, 1, "method");
  const char *method = luaL_optstring(L, -1, "GET");
  lua_pop(L, 1);

  // body
  lua_getfield(L, 1, "body");
  size_t body_len = 0;
  const char *body = nullptr;
  if (!lua_isnil(L, -1)) {
    body = lua_tolstring(L, -1, &body_len);
  }
  lua_pop(L, 1);

  // timeout
  lua_getfield(L, 1, "timeout");
  float timeout = (float)luaL_optnumber(L, -1, 30.0);
  lua_pop(L, 1);

  // output (optional file path)
  lua_getfield(L, 1, "output");
  const char *output = nullptr;
  if (!lua_isnil(L, -1)) {
    output = luaL_checkstring(L, -1);
  }
  lua_pop(L, 1);

  // override (optional, default false = resume existing file)
  lua_getfield(L, 1, "override");
  bool override_file = lua_toboolean(L, -1) != 0;
  lua_pop(L, 1);

  // count headers
  int header_count = 0;
  HttpHeader *headers_arr = nullptr;

  lua_getfield(L, 1, "headers");
  if (lua_istable(L, -1)) {
    // first pass: count
    lua_pushnil(L);
    while (lua_next(L, -2)) {
      header_count++;
      lua_pop(L, 1);
    }

    if (header_count > 0) {
      headers_arr = (HttpHeader *)::malloc(sizeof(HttpHeader) * header_count);
      int idx = 0;
      lua_pushnil(L);
      while (lua_next(L, -2)) {
        size_t klen, vlen;
        const char *k = lua_tolstring(L, -2, &klen);
        const char *v = lua_tolstring(L, -1, &vlen);
        headers_arr[idx].name = _lstrdup_malloc(k, klen);
        headers_arr[idx].value = _lstrdup_malloc(v, vlen);
        idx++;
        lua_pop(L, 1);
      }
    }
  }
  lua_pop(L, 1);

  // allocate request
  HttpRequest *req = (HttpRequest *)::calloc(1, sizeof(HttpRequest));
  req->url = _strdup_malloc(url);
  req->method = _strdup_malloc(method);
  req->body = body ? _lstrdup_malloc(body, body_len) : nullptr;
  req->body_len = body_len;
  req->headers = headers_arr;
  req->header_count = header_count;
  req->timeout_secs = timeout;
  req->output_path = output ? _strdup_malloc(output) : nullptr;
  req->output_override = override_file;
  req->state.store(0, std::memory_order_release);
  req->response_body.init();
  req->response_headers_raw.init();

  // start worker thread
  req->thread.make(_http_worker, req);

  // push as userdata
  HttpRequest **pptr =
      (HttpRequest **)lua_newuserdatauv(L, sizeof(HttpRequest *), 0);
  *pptr = req;
  luaL_setmetatable(L, HTTP_REQUEST_MT);
  return 1;
}

// spry.http.tls_available() -> bool
static int spry_http_tls_available(lua_State *L) {
#if !defined(IS_HTML5)
  char err[256] = {};
  lua_pushboolean(L, _tls_init(err, sizeof(err)));
#else
  lua_pushboolean(L, false);
#endif
  return 1;
}

// ============================================================
// Module open / shutdown
// ============================================================

void open_http_api(lua_State *L) {
  open_mt_http_request(L);

  // create spry.http table with C functions
  lua_newtable(L);

  lua_pushcfunction(L, spry_http_request);
  lua_setfield(L, -2, "_request");

  lua_pushcfunction(L, spry_http_tls_available);
  lua_setfield(L, -2, "tls_available");

  // spry.http = table
  lua_getglobal(L, "spry");
  lua_pushvalue(L, -2);
  lua_setfield(L, -2, "http");
  lua_pop(L, 2); // pop spry table and http table
}

void http_shutdown(void) {
  _tls_cleanup();
#ifdef IS_WIN32
  if (g_winsock_state.load() == 2) {
    WSACleanup();
    g_winsock_state.store(0);
  }
#endif
}
