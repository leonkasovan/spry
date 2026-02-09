# Gamepad Example

Comprehensive demonstration of Spry's joystick/gamepad API.

## Features Demonstrated

| # | Feature | API |
|---|---------|-----|
| 1 | Detecting controllers | `spry.joystick_count()`, `spry.joystick_connected(id)` |
| 2 | Analog sticks | `spry.joystick_axis(id, "leftx")` etc. |
| 3 | Button press/release/down | `spry.joystick_down/press/release(id, btn)` |
| 4 | Trigger axes | `spry.joystick_axis(id, "lefttrigger")` |
| 5 | D-pad buttons | `spry.joystick_down(id, "dpup")` etc. |
| 6 | Rumble / vibration | `spry.joystick_vibration(id, l, r, dur)` |
| 7 | Raw button/axis access | `spry.joystick_button(id, n)`, `spry.joystick_raw_axis(id, n)` |
| 8 | Capability queries | `spry.joystick_axis_count(id)`, `spry.joystick_button_count(id)` |
| 9 | Name and GUID | `spry.joystick_name(id)`, `spry.joystick_guid(id)` |
| 10 | SDL2 mapping database | `spry.joystick_add_mappings("gamecontrollerdb.txt")` |
| 11 | Deadzone config | `spry.joystick_deadzone(val)` |
| 12 | Combined keyboard+gamepad | Keyboard fallback for all gamepad actions |

## Controls

- **Left stick / Arrow keys** — move player
- **A / Space** — jump
- **B / R** — trigger rumble
- **D-pad** — nudge player
- **Start** — reset position
- **Tab** — cycle joystick slot (1–4)
- **F1** — toggle debug overlay
- **Esc** — quit

## SDL2 Mapping Database

Place a `gamecontrollerdb.txt` file next to `main.lua` to support
non-standard controllers. Download from:
https://github.com/gabomdq/SDL_GameControllerDB
