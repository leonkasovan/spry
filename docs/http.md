# spry.http — HTTP/HTTPS Client

A coroutine-friendly HTTP/HTTPS client built into Spry. Requests run on a background thread, and the high-level API yields the calling coroutine until the response arrives.

- **HTTP/1.1** with chunked transfer-encoding support.
- **HTTPS** via runtime-loaded OpenSSL (no hard dependency — HTTP still works without it).
- **Non-blocking** — the worker runs on a separate thread; Lua coroutines yield until done.

> **Important:** All high-level functions (`request`, `get`, `post`, `put`, `delete`) must be called from inside a coroutine. Use Spry's built-in `resume()` helper to drive them each frame.

---

## API Reference

### `spry.http.request(opts [, body])`

Perform a full HTTP or HTTPS request. Yields until the response is received.

**Parameters**

| Name   | Type             | Description |
|--------|------------------|-------------|
| `opts` | `table\|string`  | A URL string, or an options table (see below). |
| `body` | `string\|nil`    | Optional request body. Only used when `opts` is a plain URL string; passing a body this way automatically sets the method to `"POST"`. |

**Options table fields**

| Field     | Type     | Default  | Description |
|-----------|----------|----------|-------------|
| `url`     | `string` | *(required)* | The full URL including scheme (`http://` or `https://`). |
| `method`  | `string` | `"GET"`  | HTTP method (`"GET"`, `"POST"`, `"PUT"`, `"DELETE"`, etc.). |
| `headers` | `table`  | `nil`    | Key-value table of request headers, e.g. `{ ["Authorization"] = "Bearer ..." }`. |
| `body`    | `string` | `nil`    | Request body payload. |
| `timeout` | `number` | `30`     | Timeout in seconds. |

**Returns**

| # | Type            | Description |
|---|-----------------|-------------|
| 1 | `string\|nil`   | Response body, or `nil` on error. |
| 2 | `integer`       | HTTP status code (e.g. `200`), or `0` on error. |
| 3 | `table`         | Response headers as a table with lowercase keys. |
| 4 | `string\|nil`   | Error message, or `nil` on success. |

---

### `spry.http.get(url [, headers])`

Convenience wrapper for a GET request. Yields until complete.

**Parameters**

| Name      | Type          | Description |
|-----------|---------------|-------------|
| `url`     | `string`      | The URL to fetch. |
| `headers` | `table\|nil`  | Optional request headers. |

**Returns** — same 4-tuple as `request`.

---

### `spry.http.post(url, content_type, body [, headers])`

Convenience wrapper for a POST request. Yields until complete.

**Parameters**

| Name           | Type          | Description |
|----------------|---------------|-------------|
| `url`          | `string`      | The URL to post to. |
| `content_type` | `string`      | Value for the `Content-Type` header, e.g. `"application/json"`. |
| `body`         | `string`      | The request body. |
| `headers`      | `table\|nil`  | Optional extra headers. A `Content-Type` key here overrides `content_type`. |

**Returns** — same 4-tuple as `request`.

---

### `spry.http.put(url, content_type, body [, headers])`

Convenience wrapper for a PUT request. Yields until complete.

**Parameters**

| Name           | Type          | Description |
|----------------|---------------|-------------|
| `url`          | `string`      | The URL to send to. |
| `content_type` | `string`      | Value for the `Content-Type` header. |
| `body`         | `string`      | The request body. |
| `headers`      | `table\|nil`  | Optional extra headers. |

**Returns** — same 4-tuple as `request`.

---

### `spry.http.delete(url [, headers])`

Convenience wrapper for a DELETE request. Yields until complete.

**Parameters**

| Name      | Type          | Description |
|-----------|---------------|-------------|
| `url`     | `string`      | The URL to delete. |
| `headers` | `table\|nil`  | Optional request headers. |

**Returns** — same 4-tuple as `request`.

---

### `spry.http.tls_available()`

Check whether HTTPS is available (i.e. OpenSSL was found and loaded at runtime).

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if TLS/HTTPS is available, `false` otherwise. |

---

### `spry.http._request(opts)` *(low-level)*

Starts an HTTP request on a background thread and returns immediately with a request handle. You normally don't need this — use `request()` instead.

**Parameters** — same options table as `request`.

**Returns** — an `HttpRequest` userdata with the following methods:

| Method       | Returns          | Description |
|--------------|------------------|-------------|
| `req:done()` | `boolean`        | `true` when the request has finished (success or error). |
| `req:result()`| `body, status, headers, err` | Same 4-tuple as `request`. Only valid after `done()` returns `true`. |

The userdata is garbage-collected; the finalizer joins the worker thread and frees all memory.

---

## Common Usage

### Simple GET inside a coroutine

```lua
function spry.start()
  resume(coroutine.create(function()
    local body, status, headers, err = spry.http.get("https://httpbin.org/get")
    if err then
      print("error: " .. err)
    else
      print("status: " .. status)
      print(body)
    end
  end))
end
```

### POST JSON data

```lua
resume(coroutine.create(function()
  local json = '{"name":"spry","version":1}'
  local body, status, headers, err = spry.http.post(
    "https://httpbin.org/post",
    "application/json",
    json
  )
  if status == 200 then
    print("success: " .. body)
  end
end))
```

### Using the full options table

```lua
resume(coroutine.create(function()
  local body, status, headers, err = spry.http.request({
    url     = "https://api.example.com/data",
    method  = "POST",
    headers = {
      ["Authorization"] = "Bearer my-token",
      ["Content-Type"]  = "application/json",
    },
    body    = '{"key":"value"}',
    timeout = 10,
  })

  if err then
    print("request failed: " .. err)
  else
    print("status: " .. status)
    print("content-type: " .. (headers["content-type"] or "unknown"))
    print(body)
  end
end))
```

### Check for HTTPS support

```lua
if spry.http.tls_available() then
  print("HTTPS is available")
else
  print("HTTPS not available — only HTTP will work")
end
```

### Using the low-level `_request` API

If you want to poll manually instead of yielding:

```lua
local req = spry.http._request({ url = "http://example.com" })

function spry.frame(dt)
  if req and req:done() then
    local body, status, headers, err = req:result()
    print(status, body)
    req = nil
  end
end
```
