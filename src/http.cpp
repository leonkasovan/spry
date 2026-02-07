// ============================================================
// http.cpp — lightweight HTTP/HTTPS client for Spry
//
// Features:
//   - HTTP/1.1 with chunked transfer-encoding
//   - HTTPS via dlopen'd OpenSSL (libssl + libcrypto)
//   - Non-blocking: runs request on a worker thread
//   - Lua coroutine-friendly API
//
// The TLS layer is loaded at runtime so the binary has no hard
// link-time dependency on OpenSSL.  On systems without libssl
// (or very old builds), HTTP still works; HTTPS returns an error.
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
// TLS via dlopen (OpenSSL 1.1+)
// ============================================================
#if !defined(IS_HTML5)
#ifndef IS_WIN32
#include <dlfcn.h>
#else
// Windows: LoadLibrary / GetProcAddress
static void *dlopen(const char *name, int) {
  return (void *)LoadLibraryA(name);
}
static void *dlsym(void *handle, const char *name) {
  return (void *)GetProcAddress((HMODULE)handle, name);
}
static void dlclose(void *handle) { FreeLibrary((HMODULE)handle); }
#define RTLD_LAZY 0
#define RTLD_LOCAL 0
#endif

// opaque types — we never dereference these
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_method_st SSL_METHOD;

struct TlsLib {
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

static TlsLib _tls;

#define SSL_CTRL_SET_TLSEXT_HOSTNAME 55
#define TLSEXT_NAMETYPE_host_name 0

static bool _tls_load(void) {
  if (_tls.loaded) return true;

  // try common library names
  const char *ssl_names[] = {"libssl.so.3", "libssl.so.1.1", "libssl.so",
#ifdef IS_WIN32
                             "libssl-3-x64.dll", "libssl-1_1-x64.dll",
#endif
                             nullptr};
  const char *crypto_names[] = {
      "libcrypto.so.3", "libcrypto.so.1.1", "libcrypto.so",
#ifdef IS_WIN32
      "libcrypto-3-x64.dll", "libcrypto-1_1-x64.dll",
#endif
      nullptr};

  for (int i = 0; crypto_names[i]; i++) {
    _tls.handle_crypto =
        dlopen(crypto_names[i], RTLD_LAZY | RTLD_LOCAL);
    if (_tls.handle_crypto) break;
  }
  if (!_tls.handle_crypto) return false;

  for (int i = 0; ssl_names[i]; i++) {
    _tls.handle_ssl =
        dlopen(ssl_names[i], RTLD_LAZY | RTLD_LOCAL);
    if (_tls.handle_ssl) break;
  }
  if (!_tls.handle_ssl) {
    dlclose(_tls.handle_crypto);
    _tls.handle_crypto = nullptr;
    return false;
  }

  void *ssl = _tls.handle_ssl;
#define TLS_SYM(member, name) \
  *(void **)(&_tls.member) = dlsym(ssl, name)

  TLS_SYM(OPENSSL_init_ssl, "OPENSSL_init_ssl");
  TLS_SYM(TLS_client_method, "TLS_client_method");
  TLS_SYM(SSL_CTX_new, "SSL_CTX_new");
  TLS_SYM(SSL_CTX_free, "SSL_CTX_free");
  TLS_SYM(SSL_CTX_ctrl, "SSL_CTX_ctrl");
  TLS_SYM(SSL_new, "SSL_new");
  TLS_SYM(SSL_free, "SSL_free");
  TLS_SYM(SSL_set_fd, "SSL_set_fd");
  TLS_SYM(SSL_connect, "SSL_connect");
  TLS_SYM(SSL_read, "SSL_read");
  TLS_SYM(SSL_write, "SSL_write");
  TLS_SYM(SSL_shutdown, "SSL_shutdown");
  TLS_SYM(SSL_ctrl, "SSL_ctrl");
  TLS_SYM(SSL_get_error, "SSL_get_error");
#undef TLS_SYM

  // verify required symbols
  if (!_tls.SSL_CTX_new || !_tls.TLS_client_method || !_tls.SSL_new ||
      !_tls.SSL_connect || !_tls.SSL_read || !_tls.SSL_write) {
    dlclose(_tls.handle_ssl);
    dlclose(_tls.handle_crypto);
    _tls.handle_ssl = nullptr;
    _tls.handle_crypto = nullptr;
    return false;
  }

  // init OpenSSL
  if (_tls.OPENSSL_init_ssl) {
    _tls.OPENSSL_init_ssl(0, nullptr);
  }

  _tls.loaded = true;
  return true;
}

static void _tls_unload(void) {
  if (_tls.handle_ssl) dlclose(_tls.handle_ssl);
  if (_tls.handle_crypto) dlclose(_tls.handle_crypto);
  memset(&_tls, 0, sizeof(_tls));
}

#else // IS_HTML5
static bool _tls_load(void) { return false; }
static void _tls_unload(void) {}
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
#if !defined(IS_HTML5)
  SSL *ssl;
  SSL_CTX *ssl_ctx;
#endif
  bool is_tls;
};

static bool _conn_connect(Connection *conn, ParsedUrl *url) {
  memset(conn, 0, sizeof(*conn));
  conn->sock = INVALID_SOCK;
  conn->is_tls = url->https;

  struct addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *result = nullptr;
  int rc = getaddrinfo(url->host, url->port, &hints, &result);
  if (rc != 0 || !result) return false;

  socket_t sock = INVALID_SOCK;
  for (struct addrinfo *rp = result; rp; rp = rp->ai_next) {
    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock == INVALID_SOCK) continue;
    if (connect(sock, rp->ai_addr, (int)rp->ai_addrlen) == 0) break;
    close_socket(sock);
    sock = INVALID_SOCK;
  }
  freeaddrinfo(result);

  if (sock == INVALID_SOCK) return false;
  conn->sock = sock;

#if !defined(IS_HTML5)
  if (url->https) {
    if (!_tls_load()) {
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }

    conn->ssl_ctx = _tls.SSL_CTX_new(_tls.TLS_client_method());
    if (!conn->ssl_ctx) {
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }

    conn->ssl = _tls.SSL_new(conn->ssl_ctx);
    if (!conn->ssl) {
      _tls.SSL_CTX_free(conn->ssl_ctx);
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }

    _tls.SSL_set_fd(conn->ssl, (int)sock);

    // SNI: set hostname so virtual-hosted servers pick the right cert
    if (_tls.SSL_ctrl) {
      _tls.SSL_ctrl(conn->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME,
                     TLSEXT_NAMETYPE_host_name, (void *)url->host);
    }

    if (_tls.SSL_connect(conn->ssl) <= 0) {
      _tls.SSL_free(conn->ssl);
      _tls.SSL_CTX_free(conn->ssl_ctx);
      close_socket(sock);
      conn->sock = INVALID_SOCK;
      return false;
    }
  }
#endif

  return true;
}

static int _conn_write(Connection *conn, const void *buf, int len) {
#if !defined(IS_HTML5)
  if (conn->is_tls && conn->ssl) {
    return _tls.SSL_write(conn->ssl, buf, len);
  }
#endif
  return (int)send(conn->sock, (const char *)buf, len, 0);
}

static int _conn_read(Connection *conn, void *buf, int len) {
#if !defined(IS_HTML5)
  if (conn->is_tls && conn->ssl) {
    return _tls.SSL_read(conn->ssl, buf, len);
  }
#endif
  return (int)recv(conn->sock, (char *)buf, len, 0);
}

static void _conn_close(Connection *conn) {
#if !defined(IS_HTML5)
  if (conn->ssl) {
    _tls.SSL_shutdown(conn->ssl);
    _tls.SSL_free(conn->ssl);
    conn->ssl = nullptr;
  }
  if (conn->ssl_ctx) {
    _tls.SSL_CTX_free(conn->ssl_ctx);
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

  char error[512];

  // -- synchronization --
  std::atomic<int> state; // 0=running, 1=done, 2=error
  Thread thread;
};

// send exactly `len` bytes
static bool _send_all(Connection *conn, const char *data, int len) {
  int sent = 0;
  while (sent < len) {
    int n = _conn_write(conn, data + sent, len - sent);
    if (n <= 0) return false;
    sent += n;
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
static bool _read_exact(Connection *conn, ByteBuf *buf, u64 count) {
  buf->ensure(count);
  u64 total = 0;
  while (total < count) {
    int chunk = (int)((count - total) > 65536 ? 65536 : (count - total));
    int n = _conn_read(conn, buf->data + buf->len, chunk);
    if (n <= 0) return false;
    buf->len += n;
    total += n;
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

  ParsedUrl url;
  if (!_url_parse(req->url, &url)) {
    snprintf(req->error, sizeof(req->error), "invalid URL: %s", req->url);
    req->state.store(2, std::memory_order_release);
    return;
  }

  if (url.https && !_tls_load()) {
    snprintf(req->error, sizeof(req->error),
             "HTTPS not available (libssl not found)");
    req->state.store(2, std::memory_order_release);
    return;
  }

  Connection conn;
  if (!_conn_connect(&conn, &url)) {
    snprintf(req->error, sizeof(req->error), "connection to %s:%s failed",
             url.host, url.port);
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

  if (!_send_all(&conn, sendbuf.data, (int)sendbuf.len)) {
    snprintf(req->error, sizeof(req->error), "failed to send request");
    sendbuf.trash();
    _conn_close(&conn);
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

  while (true) {
    if (!_read_line(&conn, &line)) {
      snprintf(req->error, sizeof(req->error), "failed to read headers");
      line.trash();
      _conn_close(&conn);
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
      }
      if (name_len == 17 && ci_eq(line.data, "transfer-encoding", 17)) {
        // check if "chunked" appears in the value
        const char *ch = strstr(val, "chunked");
        if (!ch) ch = strstr(val, "Chunked");
        if (ch) chunked = true;
      }
    }
  }
  req->response_headers_raw.null_terminate();

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
      if (!_read_exact(&conn, &req->response_body, chunk_size)) {
        snprintf(req->error, sizeof(req->error),
                 "failed to read chunked body");
        line.trash();
        _conn_close(&conn);
        req->state.store(2, std::memory_order_release);
        return;
      }
      // read chunk trailing \r\n
      _read_line(&conn, &line);
    }
  } else if (content_length >= 0) {
    if (!_read_exact(&conn, &req->response_body, (u64)content_length)) {
      snprintf(req->error, sizeof(req->error), "failed to read body");
      line.trash();
      _conn_close(&conn);
      req->state.store(2, std::memory_order_release);
      return;
    }
  } else {
    // read until connection closes
    char buf[4096];
    while (true) {
      int n = _conn_read(&conn, buf, sizeof(buf));
      if (n <= 0) break;
      req->response_body.append(buf, n);
    }
  }
  req->response_body.null_terminate();

  line.trash();
  _conn_close(&conn);
  req->state.store(1, std::memory_order_release);
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
  lua_pushlstring(L, req->response_body.data, req->response_body.len);
  lua_pushinteger(L, req->status_code);
  _push_headers_table(L, req->response_headers_raw.data);
  lua_pushnil(L); // no error
  return 4;
}

static int open_mt_http_request(lua_State *L) {
  luaL_Reg reg[] = {
      {"__gc", mt_http_request_gc},
      {"done", mt_http_request_done},
      {"result", mt_http_request_result},
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
  lua_pushboolean(L, _tls_load());
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

void http_shutdown(void) { _tls_unload(); }
