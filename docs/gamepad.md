# spry.joystick — Gamepad & Joystick Input

Query connected controllers, read mapped gamepad buttons/axes, access raw input, set vibration, and load SDL2-compatible controller mappings.

- Up to **4 joysticks** simultaneously.
- Joystick indices are **1-based** in Lua (1–4).
- Recognized controllers are automatically mapped to a standard gamepad layout via an SDL2-compatible mapping database.
- A configurable deadzone is applied to mapped axes.

---

## Button Names

Used by `joystick_down`, `joystick_press`, and `joystick_release`:

| Name             | Description |
|------------------|-------------|
| `"a"`            | Face button A (bottom) |
| `"b"`            | Face button B (right) |
| `"x"`            | Face button X (left) |
| `"y"`            | Face button Y (top) |
| `"back"`         | Back / Select |
| `"guide"`        | Guide / Home |
| `"start"`        | Start / Menu |
| `"leftstick"`    | Left stick click (L3) |
| `"rightstick"`   | Right stick click (R3) |
| `"leftshoulder"` | Left bumper (LB) |
| `"rightshoulder"`| Right bumper (RB) |
| `"dpup"`         | D-pad up |
| `"dpdown"`       | D-pad down |
| `"dpleft"`       | D-pad left |
| `"dpright"`      | D-pad right |

## Axis Names

Used by `joystick_axis`:

| Name              | Description |
|-------------------|-------------|
| `"leftx"`         | Left stick X (−1 left, +1 right) |
| `"lefty"`         | Left stick Y (−1 up, +1 down) |
| `"rightx"`        | Right stick X |
| `"righty"`        | Right stick Y |
| `"lefttrigger"`   | Left trigger (0 released, +1 fully pressed) |
| `"righttrigger"`  | Right trigger |

---

## API Reference

### `spry.joystick_count()`

Returns the number of currently connected joysticks.

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Number of connected joysticks (0–4). |

---

### `spry.joystick_connected(index)`

Check whether a joystick at the given slot is connected.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `index` | `integer` | Joystick slot (1–4). |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if connected. |

---

### `spry.joystick_name(index)`

Get the human-readable name of a joystick.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `index` | `integer` | Joystick slot (1–4). |

**Returns**

| # | Type          | Description |
|---|---------------|-------------|
| 1 | `string\|nil` | Controller name, or `nil` if not connected. |

---

### `spry.joystick_guid(index)`

Get the GUID string of a joystick (used for mapping lookups).

| Name    | Type      | Description |
|---------|-----------|-------------|
| `index` | `integer` | Joystick slot (1–4). |

**Returns**

| # | Type          | Description |
|---|---------------|-------------|
| 1 | `string\|nil` | GUID, or `nil` if not connected. |

---

### `spry.joystick_is_gamepad(index)`

Check whether the joystick has been recognized as a standard gamepad (i.e. a mapping was found in the mapping database).

| Name    | Type      | Description |
|---------|-----------|-------------|
| `index` | `integer` | Joystick slot (1–4). |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if recognized as a gamepad. |

---

### `spry.joystick_down(index, button)`

Check whether a mapped gamepad button is currently held down.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `index`  | `integer` | Joystick slot (1–4). |
| `button` | `string`  | Button name (see table above). |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if held. |

---

### `spry.joystick_press(index, button)`

Check whether a mapped gamepad button was **just pressed** this frame (down now, up last frame).

| Name     | Type      | Description |
|----------|-----------|-------------|
| `index`  | `integer` | Joystick slot (1–4). |
| `button` | `string`  | Button name. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if just pressed. |

---

### `spry.joystick_release(index, button)`

Check whether a mapped gamepad button was **just released** this frame (up now, down last frame).

| Name     | Type      | Description |
|----------|-----------|-------------|
| `index`  | `integer` | Joystick slot (1–4). |
| `button` | `string`  | Button name. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if just released. |

---

### `spry.joystick_axis(index, axis)`

Read a mapped gamepad axis value (with deadzone applied).

| Name    | Type      | Description |
|---------|-----------|-------------|
| `index` | `integer` | Joystick slot (1–4). |
| `axis`  | `string`  | Axis name (see table above). |

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | Axis value, typically −1.0 to +1.0 for sticks, 0.0 to +1.0 for triggers. |

---

### `spry.joystick_button(index, button_index)`

Read a **raw** (unmapped) button by its numeric index.

| Name           | Type      | Description |
|----------------|-----------|-------------|
| `index`        | `integer` | Joystick slot (1–4). |
| `button_index` | `integer` | Raw button index (0-based). |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if pressed. |

---

### `spry.joystick_raw_axis(index, axis_index)`

Read a **raw** (unmapped) axis value by its numeric index.

| Name         | Type      | Description |
|--------------|-----------|-------------|
| `index`      | `integer` | Joystick slot (1–4). |
| `axis_index` | `integer` | Raw axis index (0-based). |

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | Raw axis value. |

---

### `spry.joystick_button_count(index)`

Get the number of raw buttons reported by the joystick.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `index` | `integer` | Joystick slot (1–4). |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Number of raw buttons. |

---

### `spry.joystick_axis_count(index)`

Get the number of raw axes reported by the joystick.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `index` | `integer` | Joystick slot (1–4). |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Number of raw axes. |

---

### `spry.joystick_vibration(index, left, right [, duration])`

Set rumble/vibration on a gamepad.

| Name       | Type      | Description |
|------------|-----------|-------------|
| `index`    | `integer` | Joystick slot (1–4). |
| `left`     | `number`  | Left motor intensity (0.0–1.0). |
| `right`    | `number`  | Right motor intensity (0.0–1.0). |
| `duration` | `number`  | Duration in seconds. `0` = until manually stopped. Default `0`. |

---

### `spry.joystick_deadzone([value])`

Get or set the global deadzone for mapped axes.

| Name    | Type     | Description |
|---------|----------|-------------|
| `value` | `number` | *(optional)* New deadzone (default is `0.15`). |

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | The current deadzone value (after setting, if any). |

---

### `spry.joystick_add_mappings(filepath)`

Load an SDL2-compatible `gamecontrollerdb.txt` mapping file. Recognized controllers will automatically use the standard gamepad layout.

| Name       | Type     | Description |
|------------|----------|-------------|
| `filepath` | `string` | Path to the mapping file (resolved through Spry VFS). |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Number of mappings added. |

---

## Common Usage

### Move a player with the left stick

```lua
function spry.frame(dt)
  local pad = 1

  if spry.joystick_connected(pad) then
    local dx = spry.joystick_axis(pad, "leftx")
    local dy = spry.joystick_axis(pad, "lefty")
    player.x = player.x + dx * player.speed * dt
    player.y = player.y + dy * player.speed * dt
  end
end
```

### Respond to button presses

```lua
function spry.frame(dt)
  local pad = 1

  if spry.joystick_press(pad, "a") then
    player:jump()
  end

  if spry.joystick_down(pad, "x") then
    player:attack()
  end

  if spry.joystick_release(pad, "b") then
    player:charge_release()
  end
end
```

### Read triggers for analog input

```lua
function spry.frame(dt)
  local pad = 1
  local brake = spry.joystick_axis(pad, "lefttrigger")
  local gas   = spry.joystick_axis(pad, "righttrigger")
  car.speed = car.speed + (gas - brake) * acceleration * dt
end
```

### Vibration feedback

```lua
-- short rumble on hit
function on_player_hit()
  spry.joystick_vibration(1, 0.6, 0.6, 0.2)
end

-- stop vibration
function on_vibration_stop()
  spry.joystick_vibration(1, 0, 0)
end
```

### Load a custom mapping database

```lua
function spry.start()
  local added = spry.joystick_add_mappings("gamecontrollerdb.txt")
  print("loaded " .. added .. " controller mappings")
end
```

### List connected joysticks

```lua
function spry.start()
  local n = spry.joystick_count()
  print(n .. " joystick(s) connected")
  for i = 1, 4 do
    if spry.joystick_connected(i) then
      print(i .. ": " .. spry.joystick_name(i) .. " [" .. spry.joystick_guid(i) .. "]")
      if spry.joystick_is_gamepad(i) then
        print("   (recognized as gamepad)")
      else
        print("   (unmapped — " .. spry.joystick_button_count(i) .. " buttons, " .. spry.joystick_axis_count(i) .. " axes)")
      end
    end
  end
end
```

### Adjust deadzone

```lua
-- tighten the deadzone
spry.joystick_deadzone(0.1)

-- read back current value
print("deadzone: " .. spry.joystick_deadzone())
```
