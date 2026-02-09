-- ============================================================
-- Gamepad Example — comprehensive showcase of spry joystick API
--
-- Demonstrates:
--   1.  Detecting connected controllers
--   2.  Reading analog stick axes
--   3.  Button polling: down / press / release
--   4.  Trigger axes (0..1)
--   5.  D-pad buttons
--   6.  Rumble / vibration
--   7.  Raw button and axis access
--   8.  Querying capabilities (axis/button counts)
--   9.  GUID and name display
--  10.  Loading SDL2 gamecontrollerdb.txt mappings
--  11.  Deadzone configuration
--  12.  A simple player character controlled by gamepad + keyboard
--
-- Controls:
--   Left stick / Arrow keys  — move the player
--   A / Space                — jump
--   B / R key                — trigger rumble
--   D-pad                    — nudge player
--   Start                    — reset player position
--   Tab                      — cycle active joystick slot
--   F1                       — toggle debug overlay
-- ============================================================

local font
local show_debug = true
local active_joy = 1  -- which joystick slot to display (1-indexed)

-- player state
local player = {
  x = 400, y = 300,
  vx = 0, vy = 0,
  speed = 200,
  size = 20,
  color = { 100, 200, 255 },
  on_ground = true,
  jump_force = -350,
  gravity = 800,
  ground_y = 450,
}

-- rumble state
local rumble_timer = 0

-- log for events
local event_log = {}
local function log_event(fmt, ...)
  local msg = fmt:format(...)
  table.insert(event_log, 1, msg)
  if #event_log > 12 then
    table.remove(event_log)
  end
end

-- ============================================================
-- spry callbacks
-- ============================================================

function spry.conf(t)
  t.window_title = "Gamepad Examples"
  t.window_width = 900
  t.window_height = 600
end

function spry.start()
  font = spry.default_font()

  -- 10. Load SDL2-compatible mapping database (if present).
  --     Download from: https://github.com/gabomdq/SDL_GameControllerDB
  if spry.file_exists("gamecontrollerdb.txt") then
    local count = spry.joystick_add_mappings("gamecontrollerdb.txt")
    log_event("Loaded %d controller mappings from gamecontrollerdb.txt", count)
  else
    log_event("No gamecontrollerdb.txt found (optional)")
  end

  -- 11. Set a custom deadzone.
  spry.joystick_deadzone(0.15)
  log_event("Deadzone set to %.2f", spry.joystick_deadzone())
end

function spry.frame(dt)
  spry.clear_color(25, 25, 35, 255)

  -- ── input handling ──────────────────────────────────────
  handle_input(dt)

  -- ── update ──────────────────────────────────────────────
  update_player(dt)
  update_rumble(dt)

  -- ── draw ────────────────────────────────────────────────
  draw_ground()
  draw_player()
  draw_hud(dt)

  if show_debug then
    draw_debug_overlay()
  end

  draw_event_log()
end

-- ============================================================
-- Input
-- ============================================================

function handle_input(dt)
  -- Toggle debug overlay.
  if spry.key_press "f1" then
    show_debug = not show_debug
  end

  -- Cycle active joystick slot.
  if spry.key_press "tab" then
    active_joy = (active_joy % 4) + 1
    log_event("Switched to joystick slot %d", active_joy)
  end

  -- Quit.
  if spry.key_down "esc" then spry.quit() end

  -- ── Movement: keyboard fallback ─────────────────────────
  local dx, dy = 0, 0
  if spry.key_down "right" then dx = dx + 1 end
  if spry.key_down "left"  then dx = dx - 1 end
  if spry.key_down "up"    then dy = dy - 0.3 end
  if spry.key_down "down"  then dy = dy + 0.3 end

  -- ── Movement: gamepad analog sticks (2) ─────────────────
  if spry.joystick_connected(active_joy) then
    local lx = spry.joystick_axis(active_joy, "leftx")
    local ly = spry.joystick_axis(active_joy, "lefty")

    -- Override keyboard if stick is deflected.
    if math.abs(lx) > 0 then dx = lx end
    if math.abs(ly) > 0 then dy = ly end
  end

  player.vx = dx * player.speed

  -- ── D-pad nudge (5) ─────────────────────────────────────
  if spry.joystick_connected(active_joy) then
    local nudge = player.speed * 0.5
    if spry.joystick_press(active_joy, "dpleft")  then player.x = player.x - nudge * dt end
    if spry.joystick_press(active_joy, "dpright") then player.x = player.x + nudge * dt end
    if spry.joystick_press(active_joy, "dpup")    then player.y = player.y - nudge * dt end
    if spry.joystick_press(active_joy, "dpdown")  then player.y = player.y + nudge * dt end
  end

  -- ── Jump: A button or Space (3) ─────────────────────────
  local jump_pressed = spry.key_press "space"
  if spry.joystick_connected(active_joy) then
    if spry.joystick_press(active_joy, "a") then
      jump_pressed = true
      log_event("Button A pressed")
    end
  end
  if jump_pressed and player.on_ground then
    player.vy = player.jump_force
    player.on_ground = false
    log_event("Jump!")
  end

  -- ── Rumble: B button or R key (6) ───────────────────────
  local rumble_pressed = spry.key_press "r"
  if spry.joystick_connected(active_joy) then
    if spry.joystick_press(active_joy, "b") then
      rumble_pressed = true
      log_event("Button B pressed — rumble!")
    end
  end
  if rumble_pressed then
    spry.joystick_vibration(active_joy, 0.8, 0.4, 0.3)
    rumble_timer = 0.3
    log_event("Rumble: left=0.8 right=0.4  dur=0.3s")
  end

  -- ── Button release detection (3) ────────────────────────
  if spry.joystick_connected(active_joy) then
    if spry.joystick_release(active_joy, "a") then
      log_event("Button A released")
    end
    if spry.joystick_release(active_joy, "b") then
      log_event("Button B released")
    end
    if spry.joystick_release(active_joy, "start") then
      log_event("Start released")
    end
  end

  -- ── Start button: reset position ────────────────────────
  if spry.joystick_connected(active_joy) then
    if spry.joystick_press(active_joy, "start") then
      player.x = 400
      player.y = 300
      player.vx = 0
      player.vy = 0
      log_event("Start pressed — position reset")
    end
  end

  -- ── Triggers as continuous input (4) ────────────────────
  if spry.joystick_connected(active_joy) then
    local lt = spry.joystick_axis(active_joy, "lefttrigger")
    local rt = spry.joystick_axis(active_joy, "righttrigger")
    -- Use triggers to tint the player color.
    player.color[1] = 100 + math.floor(lt * 155)
    player.color[3] = 255 - math.floor(rt * 155)
  end
end

-- ============================================================
-- Update
-- ============================================================

function update_player(dt)
  -- apply velocity
  player.x = player.x + player.vx * dt
  player.y = player.y + player.vy * dt

  -- gravity
  if not player.on_ground then
    player.vy = player.vy + player.gravity * dt
  end

  -- ground collision
  if player.y >= player.ground_y then
    player.y = player.ground_y
    player.vy = 0
    player.on_ground = true
  end

  -- wrap horizontally
  local w = spry.window_width()
  if player.x < -player.size then player.x = w + player.size end
  if player.x > w + player.size then player.x = -player.size end
end

function update_rumble(dt)
  if rumble_timer > 0 then
    rumble_timer = rumble_timer - dt
    if rumble_timer <= 0 then
      -- stop rumble
      spry.joystick_vibration(active_joy, 0, 0)
    end
  end
end

-- ============================================================
-- Drawing
-- ============================================================

function draw_ground()
  local w = spry.window_width()
  spry.push_color(60, 60, 80, 255)
  spry.draw_filled_rect(0, player.ground_y + player.size, w, spry.window_height())
  spry.pop_color()
end

function draw_player()
  local r, g, b = player.color[1], player.color[2], player.color[3]
  spry.push_color(r, g, b, 255)
  spry.draw_filled_rect(
    player.x - player.size,
    player.y - player.size,
    player.x + player.size,
    player.y + player.size
  )
  spry.pop_color()

  -- shadow / ground indicator
  spry.push_color(0, 0, 0, 80)
  local shadow_w = player.size * 0.8
  spry.draw_filled_rect(
    player.x - shadow_w,
    player.ground_y + player.size,
    player.x + shadow_w,
    player.ground_y + player.size + 4
  )
  spry.pop_color()
end

function draw_hud(dt)
  local y = 8
  local sz = 14

  font:draw(("FPS: %.0f"):format(1 / dt), 10, y, sz)
  y = y + sz + 2

  -- 1. Connected controllers.
  local count = spry.joystick_count()
  font:draw(("Controllers: %d  (slot %d active,  Tab to cycle)"):format(count, active_joy), 10, y, sz)
  y = y + sz + 2

  font:draw("Space/A=jump  R/B=rumble  Arrows/Stick=move  Start=reset  F1=debug", 10, y, sz)
  y = y + sz + 2

  if spry.joystick_connected(active_joy) then
    -- 4. Trigger readout.
    local lt = spry.joystick_axis(active_joy, "lefttrigger")
    local rt = spry.joystick_axis(active_joy, "righttrigger")
    font:draw(("Triggers:  L=%.2f  R=%.2f"):format(lt, rt), 10, y, sz)
    y = y + sz + 2
  end

  if rumble_timer > 0 then
    font:draw(("Rumble: %.1fs remaining"):format(rumble_timer), 10, y, sz)
  end
end

-- ── Debug overlay ─────────────────────────────────────────

function draw_debug_overlay()
  local ox = 500
  local oy = 8
  local sz = 13
  local lh = sz + 2

  spry.push_color(0, 0, 0, 160)
  spry.draw_filled_rect(ox - 6, oy - 4, spry.window_width() - 4, 380)
  spry.pop_color()

  local y = oy

  if not spry.joystick_connected(active_joy) then
    font:draw(("Slot %d: not connected"):format(active_joy), ox, y, sz)
    return
  end

  -- 9. Name and GUID.
  local name = spry.joystick_name(active_joy) or "?"
  local guid = spry.joystick_guid(active_joy) or "?"
  font:draw(("Slot %d: %s"):format(active_joy, name), ox, y, sz); y = y + lh
  font:draw(("GUID: %s"):format(guid), ox, y, sz); y = y + lh

  -- Is it a recognized gamepad?
  local is_gp = spry.joystick_is_gamepad(active_joy)
  font:draw(("Gamepad: %s"):format(is_gp and "yes" or "no (unmapped)"), ox, y, sz); y = y + lh

  -- 8. Capabilities.
  local naxes = spry.joystick_axis_count(active_joy)
  local nbtns = spry.joystick_button_count(active_joy)
  font:draw(("Raw axes: %d  buttons: %d"):format(naxes, nbtns), ox, y, sz); y = y + lh

  -- 11. Current deadzone.
  font:draw(("Deadzone: %.2f"):format(spry.joystick_deadzone()), ox, y, sz); y = y + lh
  y = y + 4

  -- 2. Analog sticks.
  font:draw("── Sticks ──", ox, y, sz); y = y + lh
  local lx = spry.joystick_axis(active_joy, "leftx")
  local ly = spry.joystick_axis(active_joy, "lefty")
  local rx = spry.joystick_axis(active_joy, "rightx")
  local ry = spry.joystick_axis(active_joy, "righty")
  font:draw(("Left  X=%+.2f  Y=%+.2f"):format(lx, ly), ox, y, sz); y = y + lh
  font:draw(("Right X=%+.2f  Y=%+.2f"):format(rx, ry), ox, y, sz); y = y + lh

  -- draw stick visualisation
  draw_stick_viz(ox + 20,  y + 30, 25, lx, ly)
  draw_stick_viz(ox + 100, y + 30, 25, rx, ry)
  y = y + 70

  -- 4. Triggers.
  local lt = spry.joystick_axis(active_joy, "lefttrigger")
  local rt = spry.joystick_axis(active_joy, "righttrigger")
  font:draw(("LT=%.2f  RT=%.2f"):format(lt, rt), ox, y, sz); y = y + lh

  -- draw trigger bars
  draw_bar(ox, y, 80, 8, lt)
  draw_bar(ox + 100, y, 80, 8, rt)
  y = y + 16

  -- 3. Named buttons.
  font:draw("── Buttons ──", ox, y, sz); y = y + lh
  local btn_names = {
    "a", "b", "x", "y",
    "leftshoulder", "rightshoulder",
    "back", "start", "guide",
    "leftstick", "rightstick",
  }
  local line = ""
  for _, bn in ipairs(btn_names) do
    if spry.joystick_down(active_joy, bn) then
      line = line .. "[" .. bn .. "] "
    end
  end
  if #line == 0 then line = "(none held)" end
  font:draw(line, ox, y, sz); y = y + lh

  -- 5. D-pad.
  local dpad = ""
  if spry.joystick_down(active_joy, "dpup")    then dpad = dpad .. "UP " end
  if spry.joystick_down(active_joy, "dpdown")  then dpad = dpad .. "DOWN " end
  if spry.joystick_down(active_joy, "dpleft")  then dpad = dpad .. "LEFT " end
  if spry.joystick_down(active_joy, "dpright") then dpad = dpad .. "RIGHT " end
  if #dpad == 0 then dpad = "(none)" end
  font:draw("D-pad: " .. dpad, ox, y, sz); y = y + lh
  y = y + 4

  -- 7. Raw buttons and axes.
  font:draw("── Raw ──", ox, y, sz); y = y + lh
  local raw_btns = ""
  for b = 0, nbtns - 1 do
    if spry.joystick_button(active_joy, b) then
      raw_btns = raw_btns .. tostring(b) .. " "
    end
  end
  if #raw_btns == 0 then raw_btns = "(none)" end
  font:draw("Raw btns: " .. raw_btns, ox, y, sz); y = y + lh

  local raw_axes_line = ""
  for a = 0, math.min(naxes - 1, 5) do
    local v = spry.joystick_raw_axis(active_joy, a)
    raw_axes_line = raw_axes_line .. ("%d=%+.1f "):format(a, v)
  end
  font:draw("Raw axes: " .. raw_axes_line, ox, y, sz); y = y + lh
end

-- ── Visualisation helpers ─────────────────────────────────

function draw_stick_viz(cx, cy, radius, sx, sy)
  -- outer ring
  spry.push_color(80, 80, 80, 255)
  spry.draw_line_circle(cx, cy, radius)
  spry.pop_color()

  -- deadzone ring
  local dz = spry.joystick_deadzone()
  spry.push_color(50, 50, 50, 255)
  spry.draw_line_circle(cx, cy, radius * dz)
  spry.pop_color()

  -- stick dot
  local px = cx + sx * radius
  local py = cy + sy * radius
  spry.push_color(255, 220, 80, 255)
  spry.draw_filled_rect(px - 3, py - 3, px + 3, py + 3)
  spry.pop_color()
end

function draw_bar(x, y, w, h, value)
  -- background
  spry.push_color(40, 40, 40, 255)
  spry.draw_filled_rect(x, y, x + w, y + h)
  spry.pop_color()

  -- fill
  local fw = w * math.max(0, math.min(1, value))
  spry.push_color(100, 200, 100, 255)
  spry.draw_filled_rect(x, y, x + fw, y + h)
  spry.pop_color()
end

-- ── Event log ─────────────────────────────────────────────

function draw_event_log()
  local sz = 12
  local x = 10
  local y = spry.window_height() - 10 - #event_log * (sz + 2)

  spry.push_color(200, 200, 200, 180)
  for _, msg in ipairs(event_log) do
    font:draw(msg, x, y, sz)
    y = y + sz + 2
  end
  spry.pop_color()
end
