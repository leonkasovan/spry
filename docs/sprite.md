# spry.sprite — Sprite Animation

Spry can load Aseprite files (`.ase`, `.aseprite`) containing sprite sheets
with animations.

## Loading

```lua
sprite = spry.sprite_load("player.ase")
```

Sprites embed their own image data — no separate image file is needed.
The returned value is a `Sprite` userdata.

---

## Methods

### `sprite:play(tag [, loop])`

Start playing an animation tag defined in Aseprite. The `tag` parameter is
the name of the animation tag set in the Aseprite editor.

```lua
sprite:play("idle")       -- play once
sprite:play("walk", true) -- loop
```

### `sprite:update(dt)`

Advance the animation by `dt` seconds. Must be called each frame for
animated sprites.

```lua
function spry.frame(dt)
  sprite:update(dt)
end
```

### `sprite:draw(x, y [, r, sx, sy, ox, oy])`

Draw the current animation frame.

```lua
sprite:draw(x, y)                          -- basic draw
sprite:draw(x, y, math.rad(45))            -- rotated
sprite:draw(x, y, 0, 2, 2)                -- scaled 2x
sprite:draw(x, y, 0, -1, 1)               -- flipped horizontally
sprite:draw(x, y, 0, 1, 1, center_x, center_y) -- with custom origin
```

### `sprite:width()` / `sprite:height()`

Return the sprite's pixel dimensions (original Aseprite canvas size).

### `sprite:set_frame(n)`

Jump to a specific frame index (0-based).

### `sprite:total_frames()`

Return the total number of frames in the sprite.

---

## Tag Handling

When no tag is playing, the sprite draws the first frame. Use `:play()` to
switch between animation tags defined in your Aseprite file.

---

## See Also

- `examples/dungeon/` — Character with idle/walk animations
- `examples/planes/` — Animated sprites in a shmup
- `examples/particle_maker/` — Sprites used in particle system
