# spry.multithreading — Threads and Channels

Spry provides basic multithreading primitives: threads, channels for
communication, and a `select` function for multiplexing.

> **HTML5 note:** Channels work on all platforms, but threads (`spry.make_thread`)
> are not available on Emscripten (JavaScript is single-threaded).

---

## Threads

### `spry.make_thread(func)`

Create a new thread that runs `func` in a separate OS thread. Returns a
`Thread` userdata.

```lua
local th = spry.make_thread(function()
  -- heavy computation here
  print("hello from thread")
end)
```

### `thread:join()`

Block until the thread finishes. Must be called to reclaim the thread's
resources.

```lua
th:join()
```

The thread is also joined when garbage-collected.

---

## Channels

Channels allow sending Lua values between threads. Supported value types:
`nil`, `boolean`, `number`, `string`, and (nested) tables of these types.
Functions, threads, and userdata cannot be sent through channels.

### `spry.make_channel()`

Create a new channel. Returns a `Channel` userdata.

### `channel:send(value)`

Send a value through the channel. Blocks until the value is received.

```lua
channel:send("hello")
channel:send(42)
channel:send({x = 10, y = 20})
```

### `channel:recv()`

Receive a value from the channel. Blocks until a value is available.

```lua
local msg = channel:recv()
```

### `channel:try_recv()`

Non-blocking receive. Returns the value immediately, or `nil` if no value
is available.

```lua
local msg = channel:try_recv()
if msg ~= nil then
  print("received:", msg)
end
```

---

## Select

### `spry.select(channel1, ...)`

Wait for data on any of the given channels. Returns the channel that has
data and the data itself.

```lua
local ch, val = spry.select(channel_a, channel_b)
print("received", val, "from", ch)
```

---

## Example: Worker Thread Pattern

```lua
local result_ch = spry.make_channel()
local work_ch = spry.make_channel()

local worker = spry.make_thread(function()
  while true do
    local job = work_ch:recv()
    if job == "quit" then break end
    result_ch:send(job:upper())
  end
end)

-- Send work
work_ch:send("hello")
work_ch:send("world")

-- Receive results
print(result_ch:recv())  -- HELLO
print(result_ch:recv())  -- WORLD

-- Shut down
work_ch:send("quit")
worker:join()
```

---

## Limitations

- Threads share the Lua state carefully: channels copy values by
  serializing/deserializing Lua types.
- Tables sent through channels are deep-copied; modifications to the
  original are not reflected in the copy.
- Functions, userdata, and threads cannot be sent through channels.
- On HTML5, channels still work (as single-threaded queues) but
  `spry.make_thread` may not work.
