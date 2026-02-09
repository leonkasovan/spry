-- ============================================================
-- HTTP Example — comprehensive showcase of spry.http
--
-- Demonstrates:
--   1. Simple GET request
--   2. GET with custom headers (auth tokens, etc.)
--   3. POST with JSON body
--   4. PUT request
--   5. DELETE request
--   6. Non-blocking polling (without coroutine helper)
--   7. Multiple concurrent requests
--   8. Error handling
--   9. Checking TLS (HTTPS) availability
--  10. Downloading and parsing JSON
--
-- All high-level helpers (get, post, put, delete, request)
-- must be called from inside a coroutine, because they yield
-- until the response arrives.  Use `resume()` each frame.
-- ============================================================

local font
local log_lines = {}
local log_scroll = 0
local demo_co         -- the main demo coroutine
local concurrent_co   -- concurrent requests demo
local polling_request -- for the non-blocking polling demo

-- ============================================================
-- Helpers
-- ============================================================

local function log(fmt, ...)
  local msg = fmt:format(...)
  print(msg)
  log_lines[#log_lines + 1] = msg
  -- keep a scrollable window of the last 200 lines
  if #log_lines > 200 then
    table.remove(log_lines, 1)
  end
end

local function separator(title)
  log("─── %s ───", title)
end

local base_urls = {
  "https://httpbin.org",
  "https://postman-echo.com",
}

local preferred_base = nil

local function build_url(base, path)
  return base .. path
end

local function request_with_fallback(opts)
  local last_err = nil
  for _, base in ipairs(base_urls) do
    local url = build_url(base, opts.path)
    local body, status, headers, err = spry.http.request({
      url = url,
      method = opts.method,
      headers = opts.headers,
      body = opts.body,
      timeout = opts.timeout,
    })
    if not err then
      preferred_base = base
      return body, status, headers, nil, base
    end
    last_err = err
    log("  Failed %s: %s", base, err)
  end
  return nil, 0, {}, last_err or "all endpoints failed", nil
end

local function get_with_fallback(path, headers, timeout)
  return request_with_fallback({
    method = "GET",
    path = path,
    headers = headers,
    timeout = timeout,
  })
end

local function post_with_fallback(path, content_type, body, headers, timeout)
  headers = headers or {}
  headers["Content-Type"] = headers["Content-Type"] or content_type
  return request_with_fallback({
    method = "POST",
    path = path,
    headers = headers,
    body = body,
    timeout = timeout,
  })
end

local function put_with_fallback(path, content_type, body, headers, timeout)
  headers = headers or {}
  headers["Content-Type"] = headers["Content-Type"] or content_type
  return request_with_fallback({
    method = "PUT",
    path = path,
    headers = headers,
    body = body,
    timeout = timeout,
  })
end

local function delete_with_fallback(path, headers, timeout)
  return request_with_fallback({
    method = "DELETE",
    path = path,
    headers = headers,
    timeout = timeout,
  })
end

-- ============================================================
-- spry callbacks
-- ============================================================

function spry.conf(t)
  t.window_title = "HTTP Examples"
  t.window_width = 900
  t.window_height = 600
end

function spry.start()
  font = spry.default_font()

  -- 9. TLS availability check (runs immediately, no coroutine needed)
  separator("TLS Check")
  if spry.http.tls_available() then
    log("TLS (HTTPS) is available — OpenSSL loaded at runtime.")
  else
    log("TLS (HTTPS) NOT available — only plain HTTP will work.")
  end

  -- kick off main demo coroutine
  demo_co = coroutine.create(run_demos)
end

function spry.frame(dt)
  -- drive coroutines
  if demo_co and coroutine.status(demo_co) ~= "dead" then
    resume(demo_co)
  end
  if concurrent_co and coroutine.status(concurrent_co) ~= "dead" then
    resume(concurrent_co)
  end

  -- 6. Non-blocking polling demo (no coroutine)
  if polling_request then
    if polling_request:done() then
      local body, status, headers, err = polling_request:result()
      if err then
        log("[poll] Error: %s", err)
      else
        log("[poll] Done! status=%d  body=%d bytes", status, #body)
      end
      polling_request = nil -- consumed
    end
  end

  -- draw the log
  spry.clear_color(30, 30, 40, 255)

  local x, y = 10, 10
  local size = 14
  font:draw("HTTP Examples  (scroll: mouse wheel)", x, y, size)
  y = y + size + 6

  font:draw("Press [1] GET  [2] POST  [3] Concurrent  [4] Poll  [5] Error", x, y, size)
  y = y + size + 10

  -- scrollable log
  local _, sy = spry.scroll_wheel()
  log_scroll = log_scroll - sy * 3
  if log_scroll < 0 then log_scroll = 0 end
  local max_scroll = math.max(0, #log_lines * (size + 2) - (spry.window_height() - y - 10))
  if log_scroll > max_scroll then log_scroll = max_scroll end

  local clip_y = y
  for i, line in ipairs(log_lines) do
    local ly = clip_y + (i - 1) * (size + 2) - log_scroll
    if ly > clip_y - size and ly < spry.window_height() then
      font:draw(line, x, ly, size)
    end
  end

  -- keyboard shortcuts
  if spry.key_press "1" then
    demo_co = coroutine.create(demo_get)
  end
  if spry.key_press "2" then
    demo_co = coroutine.create(demo_post)
  end
  if spry.key_press "3" then
    concurrent_co = coroutine.create(demo_concurrent)
  end
  if spry.key_press "4" then
    demo_non_blocking_poll()
  end
  if spry.key_press "5" then
    demo_co = coroutine.create(demo_error_handling)
  end

  if spry.key_down "esc" then spry.quit() end
end

-- ============================================================
-- Demo routines (each must run inside a coroutine)
-- ============================================================

function run_demos()
  demo_get()
  demo_post()
  demo_concurrent()
  demo_error_handling()
  separator("All demos complete")
end

-- 1. Simple GET request
function demo_get()
  separator("1. Simple GET")
  log("Requesting /get via HTTPS fallback ...")

  local body, status, headers, err, base = get_with_fallback("/get")
  if err then
    log("  Error: %s", err)
  else
    log("  Status: %d", status)
    if base then
      log("  Base: %s", base)
    end
    log("  Body length: %d bytes", #body)
    -- show first 200 chars
    log("  Body preview: %s", body:sub(1, 200))
    -- show some headers
    if headers["content-type"] then
      log("  Content-Type: %s", headers["content-type"])
    end
  end
end

-- 2. GET with custom headers
function demo_get_with_headers()
  separator("2. GET with headers")

  local body, status, headers, err, base = get_with_fallback(
    "/headers",
    {
      ["Authorization"] = "Bearer my-secret-token",
      ["X-Custom-Header"] = "SpryEngine/0.8",
    }
  )
  if err then
    log("  Error: %s", err)
  else
    log("  Status: %d", status)
    if base then
      log("  Base: %s", base)
    end
    log("  Body preview: %s", body:sub(1, 300))
  end
end

-- 3. POST with JSON body
function demo_post()
  separator("3. POST with JSON")

  local payload = spry.json_write({
    name = "Spry Engine",
    version = spry.version(),
    features = { "gamepad", "http", "physics" },
  })

  log("Sending JSON payload (%d bytes)...", #payload)

  local body, status, headers, err, base = post_with_fallback(
    "/post",
    "application/json",
    payload
  )
  if err then
    log("  Error: %s", err)
  else
    log("  Status: %d", status)
    if base then
      log("  Base: %s", base)
    end
    log("  Body preview: %s", body:sub(1, 300))
  end
end

-- 4. PUT request
function demo_put()
  separator("4. PUT request")

  local body, status, headers, err, base = put_with_fallback(
    "/put",
    "text/plain",
    "updated resource content"
  )
  if err then
    log("  Error: %s", err)
  else
    log("  Status: %d", status)
    if base then
      log("  Base: %s", base)
    end
    log("  Body length: %d", #body)
  end
end

-- 5. DELETE request
function demo_delete()
  separator("5. DELETE request")

  local body, status, headers, err, base = delete_with_fallback("/delete")
  if err then
    log("  Error: %s", err)
  else
    log("  Status: %d", status)
    if base then
      log("  Base: %s", base)
    end
    log("  Body length: %d", #body)
  end
end

-- 6. Non-blocking polling (no coroutine needed)
function demo_non_blocking_poll()
  separator("6. Non-blocking poll")
  log("Starting request, will poll in spry.frame ...")

  -- Use the low-level _request directly — returns immediately.
  local base = preferred_base or base_urls[1]
  polling_request = spry.http._request({
    url = build_url(base, "/delay/1"),
    method = "GET",
    timeout = 10,
  })
end

-- 7. Multiple concurrent requests
function demo_concurrent()
  separator("7. Concurrent requests")
  log("Firing 3 requests in parallel ...")

  -- Fire all requests at once using the low-level API.
  local base = preferred_base or base_urls[1]
  local urls = {
    build_url(base, "/get?id=1"),
    build_url(base, "/get?id=2"),
    build_url(base, "/get?id=3"),
  }

  local requests = {}
  for i, url in ipairs(urls) do
    requests[i] = spry.http._request({ url = url, method = "GET" })
  end

  -- Yield until all complete.
  local done_count = 0
  while done_count < #requests do
    done_count = 0
    for _, req in ipairs(requests) do
      if req:done() then done_count = done_count + 1 end
    end
    coroutine.yield()
  end

  -- Collect results.
  for i, req in ipairs(requests) do
    local body, status, headers, err = req:result()
    if err then
      log("  Request %d: Error: %s", i, err)
    else
      log("  Request %d: status=%d  body=%d bytes", i, status, #body)
    end
  end
end

-- 8. Error handling
function demo_error_handling()
  separator("8. Error handling")

  -- Bad hostname
  log("Trying a bad hostname ...")
  local body, status, headers, err = spry.http.get("http://this-does-not-exist.invalid/foo")
  if err then
    log("  Expected error: %s", err)
  else
    log("  Unexpected success: status=%d", status)
  end

  -- Timeout
  log("Trying a request with very short timeout ...")
  local base = preferred_base or base_urls[1]
  local body2, status2, headers2, err2 = spry.http.request({
    url = build_url(base, "/delay/10"),
    method = "GET",
    timeout = 1,  -- 1 second timeout, but server delays 10s
  })
  if err2 then
    log("  Expected timeout error: %s", err2)
  else
    log("  Status: %d", status2)
  end
end

-- 10. Download and parse JSON
function demo_json()
  separator("10. JSON parsing")

  local body, status, _, err, base = get_with_fallback("/json")
  if err then
    log("  Error: %s", err)
    return
  end

  log("  Status: %d", status)
  if base then
    log("  Base: %s", base)
  end
  local data = spry.json_read(body)
  if data then
    log("  Parsed JSON: slideshow title = %s", data.slideshow.title or "?")
    log("  Number of slides: %d", #data.slideshow.slides)
  end
end
