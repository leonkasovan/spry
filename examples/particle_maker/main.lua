-- ============================================================
-- Particle Maker Example (Spry)
-- A lightweight port inspired by a Love2D particle editor.
-- ============================================================

local font
local particle_system
local particle_settings = {}
local particle_position = { x = 0, y = 0 }
local current_particle_s = 1
local hide_print = false

local image_names = {
  "particle.png",
}

local images = {}
local load_names = {
  "default.lua",
  "fire.lua",
  "sun.lua",
}

local load_index = 1
local background = { 0, 0, 0, 255 }

local function random_range(min, max)
  return min + (max - min) * math.random()
end

local function lerp(a, b, t)
  return a + (b - a) * t
end

local function load_chunk(src, name)
  local loader = load or loadstring
  if not loader then return nil, "no loader" end
  return loader(src, name)
end

local function filter_existing(list)
  local out = {}
  for _, name in ipairs(list) do
    if spry.file_exists(name) then
      out[#out + 1] = name
    end
  end
  return out
end

local function load_images()
  for _, name in ipairs(image_names) do
    if spry.file_exists(name) then
      images[name] = spry.image_load(name)
    end
  end
end

local function read_preset(path)
  local contents, ok = spry.file_read(path)
  if not ok or not contents then return nil end
  local chunk = load_chunk(contents, "@" .. path)
  if not chunk then return nil end
  local ok2, data = pcall(chunk)
  if not ok2 or type(data) ~= "table" then return nil end
  return data
end

local function serialize_settings(s)
  local parts = { "return {\n" }
  local function add_line(key, value)
    parts[#parts + 1] = string.format("  %s = %s,\n", key, value)
  end

  local ordered_keys = {
    "name",
    "image",
    "area_spread_distribution",
    "area_spread_dx",
    "area_spread_dy",
    "buffer_size",
    "direction",
    "emission_rate",
    "emitter_lifetime",
    "insert_mode",
    "spread",
    "linear_acceleration_xmin",
    "linear_acceleration_ymin",
    "linear_acceleration_xmax",
    "linear_acceleration_ymax",
    "offsetx",
    "offsety",
    "plifetime_min",
    "plifetime_max",
    "radialacc_min",
    "radialacc_max",
    "rotation_min",
    "rotation_max",
    "size_variation",
    "spin_min",
    "spin_max",
    "spin_variation",
    "speed_min",
    "speed_max",
    "tangential_acceleration_min",
    "tangential_acceleration_max",
  }

  for _, key in ipairs(ordered_keys) do
    local v = s[key]
    if type(v) == "string" then
      add_line(key, string.format("%q", v))
    else
      add_line(key, tostring(v))
    end
  end

  local colors = {}
  for i = 1, #s.colors do
    local c = s.colors[i]
    colors[#colors + 1] = c[1] or 0
    colors[#colors + 1] = c[2] or 0
    colors[#colors + 1] = c[3] or 0
    colors[#colors + 1] = c[4] or 0
  end
  add_line("colors", "{ " .. table.concat(colors, ", ") .. " }")

  add_line("sizes", "{ " .. table.concat(s.sizes, ", ") .. " }")

  parts[#parts + 1] = "}\n"
  return table.concat(parts)
end

local function save_particle_system(s)
  if s.name == "" then return end
  local path = s.name .. ".lua"
  spry.file_write(path, serialize_settings(s))
end

local function apply_preset(s, data)
  for k, v in pairs(data) do
    if k == "colors" then
      local j = 1
      for i = 1, #v, 4 do
        s.colors[j] = { v[i], v[i + 1], v[i + 2], v[i + 3] }
        j = j + 1
      end
    elseif k == "sizes" then
      for i = 1, #v do
        s.sizes[i] = v[i]
      end
    else
      s[k] = v
    end
  end
end

local function load_particle_system(s)
  if s.load == "" or not s.load then return end
  local data = read_preset(s.load)
  if not data then return end
  apply_preset(s, data)
end

local function random_particle_system(s)
  s.name = os.date("Random_%Y%m%d_%H%M%S")
  math.randomseed(os.time())

  local maxcolor = math.random(8)
  for i = 1, maxcolor do
    s.colors[i] = { math.random(255), math.random(255), math.random(255), math.random(255) }
  end
  for i = maxcolor + 1, 8 do
    s.colors[i] = { 0, 0, 0, 0 }
  end

  s.plifetime_min = (math.random(2) == 1) and math.random(1, 5) or 0
  s.plifetime_max = (math.random(2) == 1) and (s.plifetime_min + math.random(0, 3)) or 1
  if #image_names > 0 then
    s.image = image_names[math.random(#image_names)]
  end

  s.linear_acceleration_xmin = (math.random(2) == 1) and math.random(-1000, 1000) or 0
  s.linear_acceleration_ymin = (math.random(2) == 1) and math.random(-1000, 1000) or 0
  s.linear_acceleration_xmax = (math.random(2) == 1) and math.random(-1000, 1000) or 0
  s.linear_acceleration_ymax = (math.random(2) == 1) and math.random(-1000, 1000) or 0

  local area_spread_distribution = { "None", "Normal", "Uniform" }
  s.area_spread_distribution = area_spread_distribution[math.random(#area_spread_distribution)]
  s.area_spread_dx = (math.random(2) == 1) and math.random(200) or 0
  s.area_spread_dy = (math.random(2) == 1) and math.random(200) or 0

  s.buffer_size = math.random(10000)
  s.direction = (math.random(2) == 1) and math.random(360) or 0
  s.emission_rate = math.random(10000)

  local insert_mode = { "Top", "Bottom", "Random" }
  s.insert_mode = insert_mode[math.random(#insert_mode)]

  s.radialacc_min = (math.random(2) == 1) and math.random(-2000, 2000) or 0
  s.radialacc_max = (math.random(2) == 1) and math.random(-2000, 2000) or 0

  s.rotation_min = 0
  s.rotation_max = math.random(360)

  s.spin_variation = math.random()
  s.spin_min = (math.random(2) == 1) and math.random(360) or 0
  s.spin_max = (math.random(2) == 1) and math.random(360) or 0

  s.speed_min = (math.random(2) == 1) and math.random(1000) or 0
  s.speed_max = (math.random(2) == 1) and math.random(1000) or 0

  s.tangential_acceleration_min = (math.random(2) == 1) and math.random(-1500, 1500) or 0
  s.tangential_acceleration_max = (math.random(2) == 1) and math.random(-1500, 1500) or 0

  local sizes = {
    { 1, 1, 1, 1, 0.75, 0.5, 0.25, 0 },
    { math.random(3), math.random(3), 0, 0, 0, 0, 0, 0 },
    { 0.5, 0.5, 0, 0, 0, 0, 0, 0 },
  }

  s.sizes = (math.random(2) == 1) and sizes[math.random(#sizes)] or { 1, 1, 0, 0, 0, 0, 0, 0 }
  s.size_variation = (math.random(2) == 1) and math.random() or 0
end

local function print_particle_system(s, x, y)
  local line_height = 16
  local row = 0
  local function line(text)
    font:draw(text, x, y + row * line_height, 14)
    row = row + 1
  end

  line("name: " .. tostring(s.name))
  line("image: " .. tostring(s.image))
  line("emission_rate: " .. tostring(s.emission_rate))
  line("lifetime: " .. tostring(s.plifetime_min) .. " - " .. tostring(s.plifetime_max))
  line("speed: " .. tostring(s.speed_min) .. " - " .. tostring(s.speed_max))
  line("spread: " .. tostring(s.spread))
  line("linear_accel: " .. tostring(s.linear_acceleration_xmin) .. ", " .. tostring(s.linear_acceleration_ymin))
  line("radial_accel: " .. tostring(s.radialacc_min) .. " - " .. tostring(s.radialacc_max))
  line("spin: " .. tostring(s.spin_min) .. " - " .. tostring(s.spin_max))
  line("size_variation: " .. tostring(s.size_variation))

  for i = 1, #s.colors do
    local c = s.colors[i]
    line(string.format("color%d: %d, %d, %d, %d", i, c[1], c[2], c[3], c[4]))
  end

  local sizes = {}
  for i = 1, #s.sizes do sizes[#sizes + 1] = s.sizes[i] end
  line("sizes: " .. table.concat(sizes, ", "))
end

local function random_normal(mean, stddev)
  local u1 = math.random()
  local u2 = math.random()
  local z0 = math.sqrt(-2 * math.log(u1)) * math.cos(2 * math.pi * u2)
  return mean + z0 * stddev
end

local function sample_curve(values, t)
  if #values == 0 then return 1 end
  if #values == 1 then return values[1] end

  local scaled = t * (#values - 1)
  local i = math.floor(scaled) + 1
  local frac = scaled - math.floor(scaled)
  local a = values[i]
  local b = values[math.min(i + 1, #values)]
  return lerp(a, b, frac)
end

local function size_stops(sizes)
  local out = {}
  for i = 1, 8 do
    local v = sizes[i] or 0
    out[#out + 1] = v
    if i < 8 and v == 0 and (sizes[i + 1] or 0) == 0 then
      break
    end
  end
  return out
end

local function color_stops(colors)
  local out = {}
  for i = 1, #colors do
    local c = colors[i]
    out[#out + 1] = { c[1] or 0, c[2] or 0, c[3] or 0, c[4] or 0 }
    if (c[1] or 0) == 0 and (c[2] or 0) == 0 and (c[3] or 0) == 0 and (c[4] or 0) == 0 then
      break
    end
  end
  return out
end

local function sample_color(stops, t)
  if #stops == 0 then return 255, 255, 255, 255 end
  if #stops == 1 then
    local c = stops[1]
    return c[1], c[2], c[3], c[4]
  end

  local scaled = t * (#stops - 1)
  local i = math.floor(scaled) + 1
  local frac = scaled - math.floor(scaled)
  local a = stops[i]
  local b = stops[math.min(i + 1, #stops)]

  return lerp(a[1], b[1], frac),
         lerp(a[2], b[2], frac),
         lerp(a[3], b[3], frac),
         lerp(a[4], b[4], frac)
end

local ParticleSystem = {}
ParticleSystem.__index = ParticleSystem

function ParticleSystem.new(settings, image, pos)
  local ps = setmetatable({}, ParticleSystem)
  ps.settings = settings
  ps.image = image
  ps.particles = {}
  ps.emit_accum = 0
  ps.emitter_time = 0
  ps.position = { x = pos.x, y = pos.y }
  return ps
end

function ParticleSystem:set_position(x, y)
  self.position.x = x
  self.position.y = y
end

function ParticleSystem:clear()
  self.particles = {}
  self.emit_accum = 0
  self.emitter_time = 0
end

function ParticleSystem:spawn_particle()
  local s = self.settings
  if #self.particles >= (s.buffer_size or 0) and (s.buffer_size or 0) > 0 then
    if s.insert_mode == "Bottom" then
      return
    elseif s.insert_mode == "Random" then
      table.remove(self.particles, math.random(#self.particles))
    else
      table.remove(self.particles, 1)
    end
  end

  local dx = 0
  local dy = 0
  if s.area_spread_distribution == "Uniform" then
    dx = random_range(-s.area_spread_dx / 2, s.area_spread_dx / 2)
    dy = random_range(-s.area_spread_dy / 2, s.area_spread_dy / 2)
  elseif s.area_spread_distribution == "Normal" then
    dx = random_normal(0, s.area_spread_dx / 3)
    dy = random_normal(0, s.area_spread_dy / 3)
  end

  local dir = math.rad(s.direction or 0)
  local spread = math.rad(s.spread or 0)
  local angle = dir + (math.random() - 0.5) * spread
  local speed = random_range(s.speed_min or 0, s.speed_max or 0)

  local p = {
    x = self.position.x + dx + (s.offsetx or 0),
    y = self.position.y + dy + (s.offsety or 0),
    vx = math.cos(angle) * speed,
    vy = math.sin(angle) * speed,
    ax = random_range(s.linear_acceleration_xmin or 0, s.linear_acceleration_xmax or 0),
    ay = random_range(s.linear_acceleration_ymin or 0, s.linear_acceleration_ymax or 0),
    origin_x = self.position.x,
    origin_y = self.position.y,
    radial_accel = random_range(s.radialacc_min or 0, s.radialacc_max or 0),
    tangential_accel = random_range(s.tangential_acceleration_min or 0, s.tangential_acceleration_max or 0),
    rotation = math.rad(random_range(s.rotation_min or 0, s.rotation_max or 0)),
    spin = math.rad(random_range(s.spin_min or 0, s.spin_max or 0)),
    life = 0,
    max_life = random_range(s.plifetime_min or 0, s.plifetime_max or 0.001),
    size_jitter = 1 + (math.random() * 2 - 1) * (s.size_variation or 0),
  }

  self.particles[#self.particles + 1] = p
end

function ParticleSystem:update(dt)
  local s = self.settings

  local emitter_live = true
  if s.emitter_lifetime and s.emitter_lifetime >= 0 then
    self.emitter_time = self.emitter_time + dt
    emitter_live = self.emitter_time <= s.emitter_lifetime
  end

  if emitter_live then
    local to_emit = (s.emission_rate or 0) * dt + self.emit_accum
    local count = math.floor(to_emit)
    self.emit_accum = to_emit - count
    for i = 1, count do
      self:spawn_particle()
    end
  end

  for i = #self.particles, 1, -1 do
    local p = self.particles[i]
    p.life = p.life + dt
    if p.life >= p.max_life then
      table.remove(self.particles, i)
    else
      local dx = p.x - p.origin_x
      local dy = p.y - p.origin_y
      local len = math.sqrt(dx * dx + dy * dy)
      local nx = (len > 0) and (dx / len) or 0
      local ny = (len > 0) and (dy / len) or 0

      local tx = -ny
      local ty = nx

      p.vx = p.vx + (p.ax + nx * p.radial_accel + tx * p.tangential_accel) * dt
      p.vy = p.vy + (p.ay + ny * p.radial_accel + ty * p.tangential_accel) * dt

      p.x = p.x + p.vx * dt
      p.y = p.y + p.vy * dt
      p.rotation = p.rotation + p.spin * dt * (1 + (s.spin_variation or 0))
    end
  end
end

function ParticleSystem:draw()
  local s = self.settings
  local size_values = size_stops(s.sizes)
  local color_values = color_stops(s.colors)

  for _, p in ipairs(self.particles) do
    local t = p.life / p.max_life
    local size = sample_curve(size_values, t) * p.size_jitter
    local r, g, b, a = sample_color(color_values, t)

    spry.push_color(r, g, b, a)
    if self.image then
      local w = self.image:width()
      local h = self.image:height()
      self.image:draw(p.x, p.y, p.rotation, size, size, w / 2, h / 2)
    else
      local side = size * 10
      spry.draw_filled_rect(p.x - side / 2, p.y - side / 2, side, side)
    end
    spry.pop_color()
  end
end

local function default_settings()
  local s = {}
  s.name = ""
  s.image = image_names[1] or ""
  s.area_spread_distribution = "None"
  s.area_spread_dx = 0
  s.area_spread_dy = 0
  s.buffer_size = 1000
  s.direction = 0
  s.emission_rate = 100
  s.emitter_lifetime = -1
  s.insert_mode = "Top"
  s.spread = 360
  s.linear_acceleration_xmin = 0
  s.linear_acceleration_ymin = 0
  s.linear_acceleration_xmax = 0
  s.linear_acceleration_ymax = 0
  s.offsetx = 0
  s.offsety = 0
  s.plifetime_min = 0
  s.plifetime_max = 1
  s.radialacc_min = 0
  s.radialacc_max = 0
  s.rotation_min = 0
  s.rotation_max = 0
  s.size_variation = 0
  s.spin_variation = 0
  s.spin_min = 0
  s.spin_max = 0
  s.speed_min = 0
  s.speed_max = 50
  s.tangential_acceleration_min = 0
  s.tangential_acceleration_max = 0

  s.colors = {}
  for i = 1, 8 do
    if i == 1 then s.colors[i] = { 255, 255, 255, 255 }
    elseif i == 2 then s.colors[i] = { 255, 255, 255, 0 }
    else s.colors[i] = { 0, 0, 0, 0 } end
  end

  s.sizes = {}
  for i = 1, 8 do
    s.sizes[i] = 0
  end
  s.sizes[1] = 1
  s.sizes[2] = 0.5
  s.sizes[3] = 0

  return s
end

local function rebuild_particle_system()
  local s = particle_settings[current_particle_s]
  local image = images[s.image]
  particle_system = ParticleSystem.new(s, image, particle_position)
end

function spry.conf(t)
  t.window_title = "Particle Maker"
  t.window_width = 1100
  t.window_height = 720
  t.swap_interval = 1
end

function spry.start()
  font = spry.default_font()

  image_names = filter_existing(image_names)
  load_names = filter_existing(load_names)

  load_images()

  particle_position.x = spry.window_width() / 2
  particle_position.y = spry.window_height() / 2

  particle_settings[current_particle_s] = default_settings()
  local s = particle_settings[current_particle_s]

  if #load_names > 0 then
    s.load = load_names[load_index]
    load_particle_system(s)
  end

  rebuild_particle_system()
end

function spry.frame(dt)
  if spry.platform() ~= "html5" and spry.key_down "esc" then
    spry.quit()
  end

  if spry.mouse_down(0) then
    local mx, my = spry.mouse_pos()
    particle_position.x = mx
    particle_position.y = my
    particle_system:set_position(mx, my)
  end

  particle_system:update(dt)

  spry.clear_color(background[1], background[2], background[3], background[4])
  particle_system:draw()

  font:draw(particle_settings[current_particle_s].name, spry.window_width() / 2 - 60, 8, 16)
  font:draw("FPS: " .. math.floor(1 / dt), spry.window_width() - 90, 10, 14)
  font:draw("F1: toggle info  F2: save  F3: random  PgUp/PgDn: load", 10, spry.window_height() - 20, 14)

  if not hide_print then
    print_particle_system(particle_settings[current_particle_s], 10, 10)
  end

  if spry.key_press "f1" then
    hide_print = not hide_print
  elseif spry.key_press "f2" then
    save_particle_system(particle_settings[current_particle_s])
  elseif spry.key_press "f3" or spry.key_press "enter" then
    random_particle_system(particle_settings[current_particle_s])
    particle_settings[current_particle_s].load = "Random.lua"
    rebuild_particle_system()
  elseif spry.key_press "pageup" or spry.key_press "=" or spry.key_press "+" then
    if #load_names > 0 then
      load_index = load_index + 1
      if load_index > #load_names then load_index = 1 end
      particle_settings[current_particle_s].load = load_names[load_index]
      load_particle_system(particle_settings[current_particle_s])
      rebuild_particle_system()
    end
  elseif spry.key_press "pagedown" or spry.key_press "-" or spry.key_press "_" then
    if #load_names > 0 then
      load_index = load_index - 1
      if load_index < 1 then load_index = #load_names end
      particle_settings[current_particle_s].load = load_names[load_index]
      load_particle_system(particle_settings[current_particle_s])
      rebuild_particle_system()
    end
  end
end
