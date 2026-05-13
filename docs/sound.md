# spry.sound — Audio Playback

Spry uses miniaudio for sound playback. Supported formats depend on the
miniaudio build, but typically include WAV, Ogg Vorbis, MP3, and FLAC.

## Loading

```lua
sound = spry.sound_load("explosion.ogg")
```

## Playback

### `sound:start()`

Start or restart playback.

```lua
sound:start()
```

### `sound:stop()`

Stop playback. The sound can be restarted with `:start()`.

### `sound:seek(pos)`

Jump to `pos` seconds.

```lua
sound:seek(10)  -- seek to 10 seconds
```

### `sound:pos()` / `sound:set_pos(pos)`

Get or set the current playback position in seconds.

---

## Properties

| Method | Description |
|--------|-------------|
| `sound:vol()` | Get current volume (0–1). |
| `sound:set_vol(v)` | Set volume (0–1). |
| `sound:pan()` | Get current pan (-1 left, 0 center, 1 right). |
| `sound:set_pan(p)` | Set pan. |
| `sound:pitch()` | Get current pitch multiplier (1 = normal). |
| `sound:set_pitch(p)` | Set pitch. |
| `sound:loop()` | Get looping state (boolean). |
| `sound:set_loop(bool)` | Enable/disable looping. |
| `sound:frames()` | Total number of audio frames. |
| `sound:secs()` | Total duration in seconds. |
| `sound:dir()` | Get direction vector (for 3D positioning). |
| `sound:set_dir(x, y, z)` | Set direction vector. |
| `sound:vel()` | Get velocity vector (for doppler). |
| `sound:set_vel(x, y, z)` | Set velocity vector. |
| `sound:set_fade(vol_start, vol_end, secs)` | Fade from `vol_start` to `vol_end` over `secs` seconds. |

---

## Timing

```lua
-- Fade in over 2 seconds
sound:set_fade(0, 1, 2)
sound:start()
```

---

## See Also

- `examples/sampler/` — Sound playback example
- `examples/jump/` — Sound effects in a platformer
