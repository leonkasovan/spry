# Bootstrap Lua API

Spry's `bootstrap.lua` is embedded in the executable and loaded before any
game scripts. It provides a class system, data types, utilities, and timer
helpers accessible globally (no `spry.` prefix needed).

---

## Class System

### `class(name [, parent])`

Creates a class with optional inheritance. Classes support hot reloading —
re-defining a class updates existing instances.

```lua
class "Animal"

function Animal:init(name)
  self.name = name
end

function Animal:speak()
  print(self.name .. " makes a sound")
end

class "Dog" : Animal

function Dog:speak()
  print(self.name .. " barks")
end
```

### `Object`

Base class for all classes created with `class()`. Provides:

| Method | Description |
|--------|-------------|
| `obj:init(...)` | Constructor (override in subclasses). Called automatically by `ClassName(...)`. |
| `obj:kill()` | Removes the object from its parent `World` (if any). |
| `obj:update(dt)` | Called each frame if the object belongs to a `World`. |
| `obj:draw()` | Called each frame if the object belongs to a `World`. |

### `super([obj, ...])`

Calls the overridden parent method from a child class. The first argument is
the method name (string), followed by any arguments.

```lua
function Dog:init(name)
  super("init", self, name)
  self.breed = "unknown"
end
```

---

## Types

### `vec2(x, y)`

2D vector class with operator overloads.

```lua
local a = vec2(10, 20)
local b = vec2(5, 5)
print(a + b)   -- vec2(15, 25)
print(a - b)   -- vec2(5, 15)
print(a * 2)   -- vec2(20, 40)
print(a / 2)   -- vec2(5, 10)
print(-a)      -- vec2(-10, -20)
print(a == b)  -- false
```

**Properties:** `x`, `y` (read/write).  
**Implicit string conversion:** `tostring(v)` returns `"vec2(x, y)"`.

---

## Entity Management

### `World()`

A container that manages a group of entities. Iterates and draws them
automatically with z-order sorting.

```lua
world = World()
player = Player(100, 100)
enemy  = Enemy(300, 200)

-- Objects added automatically via Player(...)
-- Objects are z-sorted by their `z` property (default 0)

function spry.frame(dt)
  world:update(dt)
  world:draw()
end
```

**Methods:**

| Method | Description |
|--------|-------------|
| `world:add(obj)` | Add an object to the world. |
| `world:kill(obj)` | Remove and mark an object for cleanup. |
| `world:update(dt)` | Call `update(dt)` on all alive objects. |
| `world:draw()` | Call `draw()` on all alive objects, sorted by `z`. |

### `ECS()`

Entity Component System. A data-oriented alternative to `World`.

```lua
ecs = ECS()

-- Define component types
ecs:define("position", {"x", "y"})
ecs:define("velocity", {"x", "y"})
ecs:define("sprite", {"img", "x", "y", "w", "h"})

-- Create entities with components
local e = ecs:entity {
  position = {x = 100, y = 200},
  velocity = {x = 10, y = 0},
}

-- Query entities by component
for eid, pos, vel in ecs:query("position", "velocity") do
  pos.x = pos.x + vel.x * dt
  pos.y = pos.y + vel.y * dt
end
```

**Methods:**

| Method | Description |
|--------|-------------|
| `ecs:define(name, fields)` | Register a component type with the given field names. |
| `ecs:entity(components)` | Create an entity. `components` is a table mapping component names to value tables. |
| `ecs:query(...)` | Iterate entities having all specified components. Yields `entity_id, comp1, comp2, ...` |

---

## Animators

### `Spring(target, stiffness, damping)`

A spring-damper system for smooth interpolation (useful for camera follow,
UI animations, etc.).

```lua
local camera_x = Spring(0, 4, 1)

function spry.frame(dt)
  camera_x:update(dt)
  -- camera_x.pos holds current position
end

-- Snap to a new target:
camera_x.target = 200
```

**Properties:** `pos` (current), `target`, `velocity`, `stiffness`, `damping`.  
**Methods:** `spring:update(dt)` advances the simulation.

---

## Timers

### `interval(callback, seconds [, reps])`

Run `callback` repeatedly every `seconds`. Returns a timer id.

```lua
timer_id = interval(function()
  spawn_enemy()
end, 2.0, 5)  -- 5 times
```

### `timeout(callback, seconds)`

Run `callback` once after `seconds`. Returns a timer id.

```lua
timeout(function()
  print("3 seconds elapsed")
end, 3.0)
```

### `stop_interval(id)` / `stop_timeout(id)`

Cancel a running timer.

---

## Utility Functions

| Function | Description |
|----------|-------------|
| `clamp(val, lo, hi)` | Clamp a number between `lo` and `hi`. |
| `sign(x)` | Return `-1`, `0`, or `1`. |
| `lerp(a, b, t)` | Linear interpolation between `a` and `b`. |
| `direction(x1, y1, x2, y2)` | Unit vector from (x1,y1) to (x2,y2). Returns `dx, dy`. |
| `heading(x, y)` | Angle of vector (x,y) in radians. |
| `delta_angle(a, b)` | Shortest angular difference between two angles. |
| `distance(x1, y1, x2, y2)` | Euclidean distance. |
| `normalize(x, y)` | Normalize a vector. Returns `nx, ny` (or `0,0` if length is 0). |
| `dot(x1, y1, x2, y2)` | Dot product. |
| `random(lo, hi)` | Random float in [lo, hi] (calls `love.math.random` semantics). |
| `stringify(x)` | Deep-print any Lua value (debug helper). |
| `aabb_overlap(ax, ay, aw, ah, bx, by, bw, bh)` | Check AABB overlap. |
| `rect_overlap(ax, ay, aw, ah, bx, by, bw, bh)` | Alias for `aabb_overlap`. |
| `clone(t)` | Deep copy a table. |
| `push(t, v)` | Append value `v` to array `t`. Returns the new length. |
| `map(t, f)` | Transform array `t` with function `f`. Returns a new array. |
| `filter(t, f)` | Filter array `t` with predicate `f`. Returns a new array. |
| `zip(a, b)` | Combine two arrays into an array of `{a[i], b[i]}` pairs. |
| `choose(t)` | Pick a random element from array `t`. |
| `find(t, v)` | Find first index of `v` in array `t`. Returns index or `nil`. |
| `sortpairs(t)` | Iterate over a table's keys in sorted order. Like `pairs()` but sorted. |

---

## Coroutines

### `resume(co [, ...])`

Resumes a coroutine with arguments. Unlike `coroutine.resume`, this function
propagates errors (calls `fatal_error` instead of returning `false`).

```lua
local co = coroutine.create(function()
  local body = spry.http.get("https://example.com")
  print(body)
end)

function spry.frame(dt)
  resume(co)  -- drive coroutine each frame
end
```

### `sleep(seconds)`

Yield the current coroutine for `seconds` of game time. Must be called from
inside a coroutine driven by `resume()`.

```lua
local co = coroutine.create(function()
  print("before sleep")
  sleep(2.0)
  print("after sleep")
end)
```
