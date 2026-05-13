# Networking — LuaSocket

Spry bundles [LuaSocket](https://github.com/lunarmodules/luasocket) for
low-level TCP and UDP socket networking. The `socket` module and its
submodules (`socket.http`, `socket.url`, etc.) are available globally.

> For HTTP/HTTPS requests, prefer the built-in `spry.http` API
> (`docs/http.md`) — it runs on a background thread and is
> coroutine-friendly.

---

## Availability

On HTML5 and Android, luasocket is not available (excluded from the build).
Check `spry.platform()` at runtime to determine availability.

```lua
if spry.platform() ~= "html5" and spry.platform() ~= "android" then
  local socket = require("socket")
  -- ...
end
```

---

## UDP Example (Client)

```lua
local socket = require("socket")

local udp = socket.udp()
udp:setpeername("127.0.0.1", 12345)
udp:settimeout(0)  -- non-blocking

-- Send
udp:send("hello")

-- Receive (non-blocking)
local data, err = udp:receive()
if data then
  print("received:", data)
end
```

---

## UDP Example (Server)

```lua
local socket = require("socket")

local udp = socket.udp()
udp:setsockname("*", 12345)
udp:settimeout(0)

while true do
  local data, addr, port = udp:receivefrom()
  if data then
    print("from", addr, port, ":", data)
    udp:sendto("ack", addr, port)
  end
end
```

---

## Available Modules

| Module | Description |
|--------|-------------|
| `socket` | Core socket module (TCP, UDP, DNS, timeouts). |
| `socket.http` | HTTP client (simple requests). |
| `socket.url` | URL parsing/encoding utilities. |
| `socket.headers` | HTTP header manipulation. |
| `mime` | MIME encoding/decoding (base64, quoted-printable). |
| `ltn12` | LTN12 filter/sink/source framework. |

---

## See Also

- `examples/networking/` — UDP client-server multiplayer example
