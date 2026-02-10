# HTTP Example

Comprehensive demonstration of Spry's HTTP/HTTPS client API.

## Features Demonstrated

| # | Feature | API |
|---|---------|-----|
| 1 | Simple GET | `spry.http.get(url)` |
| 2 | GET with headers | `spry.http.get(url, headers)` |
| 3 | POST with JSON | `spry.http.post(url, content_type, body)` |
| 4 | PUT request | `spry.http.put(url, content_type, body)` |
| 5 | DELETE request | `spry.http.delete(url)` |
| 6 | Non-blocking polling | `spry.http._request(opts)` + `req:done()` / `req:result()` |
| 6a | Download with progress | `req:progress()` + microui panel with URL input, progress bar |
| 7 | Concurrent requests | Fire multiple `_request()`, poll all, collect |
| 8 | Error handling | Bad hostnames, timeouts |
| 9 | TLS availability | `spry.http.tls_available()` |
| 10 | JSON round-trip | `spry.json_write()` / `spry.json_read()` + HTTP |

## Controls

- **1** — run GET demo
- **2** — run POST demo
- **3** — run concurrent requests demo
- **4** — fire non-blocking poll
- **5** — run error handling demo
- **6** — toggle download progress panel (type a URL, click Download)
- **Mouse wheel** — scroll log
- **Esc** — quit

## Notes

- High-level helpers (`get`, `post`, `put`, `delete`, `request`) **must**
  run inside a coroutine — they yield until the response arrives.
- The low-level `spry.http._request(opts)` returns immediately and can be
- The low-level `spry.http._request(opts)` returns immediately and can be
  polled each frame with `req:done()`.
- You can query live progress with `req:progress()` to get `downloaded`,
  `uploaded`, and `total` (content length when known).
- Uses [httpbin.org](http://httpbin.org) for testing (requires internet).
