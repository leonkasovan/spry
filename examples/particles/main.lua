-- ============================================================
-- Particle Effect Example
-- Click and drag the mouse to spawn colorful particles!
-- ============================================================

local particles = {}
local font

-- Particle configuration
local GRAVITY = 300
local PARTICLE_SIZE = 4
local MAX_LIFETIME = 2.0
local SPAWN_RATE = 5  -- particles per frame when mouse is held

function spry.conf(t)
  t.window_title = "Particle Effects"
  t.window_width = 1000
  t.window_height = 700
end

-- Create a new particle
local function create_particle(x, y, vx, vy, color)
  return {
    x = x,
    y = y,
    vx = vx or 0,
    vy = vy or 0,
    life = 0,
    max_life = MAX_LIFETIME * (0.8 + math.random() * 0.4),
    size = PARTICLE_SIZE * (0.7 + math.random() * 0.6),
    color = color or {
      math.random(100, 255),
      math.random(100, 255),
      math.random(100, 255),
    },
  }
end

-- Spawn particles in a burst
local function spawn_burst(x, y, count, speed, color)
  for i = 1, count do
    local angle = (i / count) * math.pi * 2
    local velocity = speed * (0.7 + math.random() * 0.6)
    local vx = math.cos(angle) * velocity
    local vy = math.sin(angle) * velocity
    table.insert(particles, create_particle(x, y, vx, vy, color))
  end
end

-- Spawn particles with random spread
local function spawn_spray(x, y, count, spread, vy_offset)
  for i = 1, count do
    local vx = (math.random() - 0.5) * spread
    local vy = (math.random() - 0.5) * spread + (vy_offset or -200)
    table.insert(particles, create_particle(x, y, vx, vy))
  end
end

function spry.start()
  font = spry.default_font()
  
  -- Spawn some initial particles for a nice effect
  local cx, cy = spry.window_width() / 2, spry.window_height() / 2
  spawn_burst(cx, cy, 100, 300, {255, 200, 100})
end

function spry.frame(dt)
  -- Background
  spry.clear_color(20, 20, 30, 255)
  
  -- Update particles
  for i = #particles, 1, -1 do
    local p = particles[i]
    
    -- Update lifetime
    p.life = p.life + dt
    if p.life > p.max_life then
      table.remove(particles, i)
    else
      -- Apply physics
      p.vy = p.vy + GRAVITY * dt
      p.x = p.x + p.vx * dt
      p.y = p.y + p.vy * dt
      
      -- Apply air resistance
      p.vx = p.vx * 0.99
      p.vy = p.vy * 0.998
    end
  end
  
  -- Spawn particles when mouse is held down
  if spry.mouse_down(0) then
    local mx, my = spry.mouse_pos()
    spawn_spray(mx, my, SPAWN_RATE, 400)
  end
  
  -- Spawn burst on right click
  if spry.mouse_click(1) then
    local mx, my = spry.mouse_pos()
    spawn_burst(mx, my, 30, 400, {100, 200, 255})
  end
  
  -- Draw particles
  for _, p in ipairs(particles) do
    local life_ratio = p.life / p.max_life
    local alpha = (1 - life_ratio) * 255
    local size = p.size * (1 - life_ratio * 0.5)
    
    spry.push_color(p.color[1], p.color[2], p.color[3], alpha)
    spry.draw_filled_rect(p.x - size/2, p.y - size/2, size, size)
    spry.pop_color()
  end
  
  -- Draw UI
  spry.push_color(255, 255, 255, 200)
  font:draw("Particle System Demo", 10, 10, 24)
  font:draw("Left Click & Drag: Spray particles", 10, 40, 16)
  font:draw("Right Click: Burst effect", 10, 60, 16)
  font:draw("Space: Explosion | F: Fireworks | A: Fire | C: Clear", 10, 80, 16)
  font:draw("Particle Count: " .. #particles, 10, 110, 16)
  font:draw("FPS: " .. math.floor(1/dt), 10, 130, 16)
  spry.pop_color()
  
  -- Keyboard shortcuts for different effects
  if spry.key_press "space" then
    -- Explosion at center
    local cx, cy = spry.window_width() / 2, spry.window_height() / 2
    spawn_burst(cx, cy, 100, 500, {255, 100, 50})
  end
  
  if spry.key_press "c" then
    -- Clear all particles
    particles = {}
  end
  
  if spry.key_press "f" then
    -- Firework effect
    for i = 1, 5 do
      local x = math.random(100, spry.window_width() - 100)
      local y = math.random(100, spry.window_height() - 100)
      local color = {
        math.random(150, 255),
        math.random(150, 255),
        math.random(150, 255),
      }
      spawn_burst(x, y, 40, 300, color)
    end
  end
  
  if spry.key_press "a" then
    -- Fire effect - spawn flames along the bottom
    local bottom_y = spry.window_height() - 50
    for i = 1, 10 do
      local x = math.random(100, spry.window_width() - 100)
      -- Fire colors: red, orange, yellow
      local fire_colors = {
        {255, 50, 0},    -- red
        {255, 120, 0},   -- orange
        {255, 200, 50},  -- yellow
      }
      local color = fire_colors[math.random(1, #fire_colors)]
      
      -- Flames rise upward with some horizontal spread
      for j = 1, 8 do
        local vx = (math.random() - 0.5) * 80
        local vy = -math.random(150, 300)  -- upward velocity
        table.insert(particles, create_particle(x, bottom_y, vx, vy, color))
      end
    end
  end
end
