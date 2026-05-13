# spry.physics — Box2D Physics

Spry integrates Box2D for 2D physics simulation: rigid bodies, collision
detection, and contact callbacks.

---

## World

### `spry.b2_world(gravity_x, gravity_y [, scale])`

Create a physics world. `scale` is the pixel-per-meter ratio (default 32).

```lua
world = spry.b2_world(0, 980)  -- Earth gravity downward
```

### `world:step(dt [, velocity_iterations, position_iterations])`

Advance the simulation. Call once per frame.

```lua
function spry.frame(dt)
  world:step(dt)
end
```

### `world:destroy()`

Destroy the world and all its bodies and fixtures.

---

## Bodies

### `world:make_static_body(x, y)` / `world:make_kinematic_body(x, y)` / `world:make_dynamic_body(x, y)`

Create a body at position (x, y).

```lua
local ground = world:make_static_body(400, 500)
local player = world:make_dynamic_body(400, 100)
```

### Body Properties

| Method | Description |
|--------|-------------|
| `body:position()` | Get `x, y` position. |
| `body:set_position(x, y)` | Set position. |
| `body:velocity()` | Get `vx, vy` velocity. |
| `body:set_velocity(vx, vy)` | Set velocity. |
| `body:angle()` | Get angle in radians. |
| `body:set_angle(a)` | Set angle. |
| `body:linear_damping()` | Get damping value. |
| `body:set_linear_damping(d)` | Set damping. |
| `body:fixed_rotation()` | Get fixed rotation state. |
| `body:set_fixed_rotation(bool)` | Enable/disable rotation. |
| `body:apply_force(fx, fy)` | Apply force at center of mass. |
| `body:apply_impulse(ix, iy)` | Apply impulse at center of mass. |
| `body:draw_fixtures()` | Debug-draw body's fixtures. |
| `body:udata(value)` | Attach custom data (string or number) to body. |
| `body:destroy()` | Remove body from world. |

---

## Fixtures

### `body:make_box_fixture(w, h [, density, friction, restitution])`

Add a rectangular fixture to a body.

```lua
body:make_box_fixture(32, 32, 1.0, 0.3, 0.1)
```

### `body:make_circle_fixture(radius [, density, friction, restitution])`

Add a circular fixture.

```lua
body:make_circle_fixture(16, 1.0, 0.5, 0.0)
```

### Fixture Properties

| Method | Description |
|--------|-------------|
| `fixture:friction()` / `fixture:set_friction(f)` | Get/set friction. |
| `fixture:restitution()` / `fixture:set_restitution(r)` | Get/set restitution (bounciness). |
| `fixture:is_sensor()` / `fixture:set_sensor(bool)` | Get/set sensor flag (detect overlap without collision). |
| `fixture:body()` | Get the parent body. |
| `fixture:udata(value)` | Attach custom data (string or number) to fixture. |

---

## Collision Callbacks

Set contact callbacks on the world for collision detection.

### `world:begin_callback(func)` / `world:end_callback(func)`

Register a function called when two fixtures begin/end touching.

```lua
function spry.start()
  world:begin_callback(function(fixture_a, fixture_b)
    print("collision started!")
  end)
  world:end_callback(function(fixture_a, fixture_b)
    print("collision ended!")
  end)
end
```

The callback receives the two `b2_fixture` userdata values. Use
`fixture:udata()` and `fixture:body():udata()` to identify what collided.

---

## Pixel-to-Meter Conversion

Box2D works in meters internally. The `scale` parameter in `b2_world()`
controls the conversion: 1 meter = `scale` pixels.

- Default scale is 32 pixels per meter.
- Gravity values should be in pixels/s² when using the scale.

---

## See Also

- `examples/boxes/` — Falling and stacked boxes
- `examples/jump/` — Platformer with physics
- `examples/dungeon/` — Tilemap collision
