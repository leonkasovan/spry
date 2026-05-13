# Networking Example

Demonstrates Spry's luasocket integration for UDP client-server networking.

## Features Demonstrated

| # | Feature | API |
|---|---------|-----|
| 1 | UDP server | `socket.udp()` with `:setsockname()` |
| 2 | UDP client | `socket.udp()` with `:setpeername()` |
| 3 | Message sending | `:send()` |
| 4 | Message receiving | `:receive()` |
| 5 | Non-blocking I/O | `:settimeout(0)` |
| 6 | Multiplayer game state | Syncing player positions |

## Structure

- **server/main.lua** — UDP server that relays player positions
- **client/main.lua** — UDP client that sends/receives position data

## How to Run

1. Start the server:
   ```
   spry examples/networking/server
   ```
2. Start one or more clients (each in a separate terminal):
   ```
   spry examples/networking/client
   ```

## Controls

- **Arrow keys** — move player
- Close window or Esc — quit
