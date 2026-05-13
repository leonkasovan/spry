# Spry TODO

A prioritized task list for improving the Spry 2D game framework.

---

> **Status:** All items in sections 1, 2, 4, 5, and the following are done:
> Sections 3.x and 2.6 remain (feature gaps and minor polish).

---

## 1. Bugs (Critical) ✓

### 1.1 `spry_file_write()` inverted success check
- **Fixed:** `src/api.cpp` — changed `written < contents.len` to `written == contents.len`.

### 1.2 `spry_pop_matrix()` wrong error message and dead code
- **Fixed:** `src/api.cpp` — error now says "matrix stack is empty", dead `return 0;` removed from both push and pop.

### 1.3 `spry_platform()` undefined behavior on Android / unknown platforms
- **Fixed:** `src/api.cpp` — added `IS_ANDROID` branch returning `"android"` and `#else` returning `"unknown"`.

### 1.4 Tilemap A* uses `world_x` instead of `world_y` for Y coordinate
- **Fixed:** `src/tilemap.cpp` — changed `world_x` to `world_y`.

### 1.5 `LuaVariant::trash()` table→userdata fall-through frees uninitialized pointer
- **Fixed:** `src/concurrency.cpp` — added `break;` before the `LUA_TUSERDATA` case.

---

## 2. Code Quality

### 2.1 Remove debug `printf` calls from release builds ✓
- **Fixed:** `src/image.cpp`, `src/sprite.cpp`, `src/atlas.cpp`, `src/tilemap.cpp`, `src/font.cpp`, `src/assets.cpp` — wrapped all debug `printf` calls in `#ifndef NDEBUG`.

### 2.2 Unreachable `return 0` in matrix stack functions ✓
- **Fixed:** `src/api.cpp` — removed dead `return 0;` from both `spry_push_matrix` and `spry_pop_matrix`.

### 2.3 Microui character input truncation breaks Unicode ✓
- **Fixed:** `src/microui.cpp` — replaced `char_code % 256` with proper UTF-8 encoding for codepoints up to 0x10FFFF.

### 2.4 `Atlas::load` debug print was fine
- **No change needed:** HashMap's `load` field IS the live entry count (just misleadingly named). Originally changed to `len` but that field doesn't exist — reverted.

### 2.5 `g_app->default_font` lazy initialization in multiple places ✓
- **Fixed:** `src/app.h` — added `load_default_font()` helper. Updated `src/main.cpp` and `src/api.cpp` to use it.

### 2.6 Audio configuration from spry.conf() ✓
- **Fixed:** `src/main.cpp` and `src/app.h` — `audio_channels` and `audio_sample_rate` can now be set via `spry.conf()`.

### 2.7 HTTP SChannel debug logging guard ✓
- **Fixed:** `src/http.cpp` — wrapped all `fprintf(stderr, "[TLS]..."` calls behind `TLS_DEBUG()` macro that compiles to nothing in release builds (`#ifdef NDEBUG`).

### 2.8 HTTP `timeout_secs` field enforcement ✓
- **Fixed:** `src/http.cpp` — added `_timeout_expired()` helper and timeout checks in the redirect loop, body read loops (chunked, content-length, read-until-close), and HTTP debug logging guarded.

---

## 3. Feature Gaps

### 3.1 Box2D joints — distance and revolute ✓
- **Added:** `mt_b2_joint` metatype with `__gc`, `destroy`, `type`, `body_a`, `body_b`.
- **Added:** `world:make_distance_joint(body_a, body_b, opts)`.
- **Added:** `world:make_revolute_joint(body_a, body_b, opts)`.
- Remaining joint types (prismatic, weld, wheel, mouse, gear, pulley, friction, motor) still need creation bindings.

### 3.2 Box2D raycasting ✓
- **Added:** `world:raycast(x1, y1, x2, y2)` — returns `fixture, px, py, nx, ny` of the closest hit, or `nil`.

### 3.3 Box2D PreSolve / PostSolve ✓
- **Added:** `world:presolve(func)` — function receives `(fixture_a, fixture_b)`, return `false` to disable contact.
- **Added:** `world:postsolve(func)` — function receives `(fixture_a, fixture_b, impulses)`.

### 3.4 Shader / custom pipeline support
- **Pending:** No way to create custom shaders from Lua.

### 3.5 Framebuffer / render target for offscreen rendering
- **Pending:** No offscreen rendering support.

### 3.6 Sound pooling / caching ✓
- **Fixed:** `src/sound.cpp`, `src/sound.h` — added refcounting with `sound_unref()`, a `HashMap<Sound*>` cache keyed by path hash, and `sound_cache_trash()`.

### 3.7 Android gamepad input (stubbed)
- **Pending:** `src/gamepad.cpp` — gamepad stubs for Android.

### 3.8 Android touchscreen input
- **Pending:** No touch event handling exposed to Lua.

---

## 4. Documentation ✓

### 4.1 Bootstrap Lua API reference
- **Created:** `docs/bootstrap.md` — class system, vec2, World, ECS, Spring, timers, utilities, coroutines.

### 4.2 Microui module documentation
- **Created:** `docs/microui.md` — windows, panels, layout, widgets, ref system, style.

### 4.3 Sprite system documentation
- **Created:** `docs/sprite.md` — loading, play/update/draw, frame control, Aseprite integration.

### 4.4 Sound system documentation
- **Created:** `docs/sound.md` — loading, playback, volume/pan/pitch, looping, fade, 3D positioning.

### 4.5 Box2D (physics) documentation
- **Created:** `docs/physics.md` — world, bodies, fixtures, collision callbacks, coordinate system.

### 4.6 Tilemap / LDtk documentation
- **Created:** `docs/tilemap.md` — loading, rendering, entities, collision, A* pathfinding.

### 4.7 Multithreading documentation
- **Created:** `docs/multithreading.md` — threads, channels, select, patterns, limitations.

### 4.8 Networking / LuaSocket documentation
- **Created:** `docs/networking.md` — UDP client/server, available modules, platform notes.

### 4.9 Website guide pages
- **Pending:** Guide pages for website/PHP need separate effort.

### 4.10 THIRD_PARTY.md with dependency versions and licenses
- **Created:** `THIRD_PARTY.md` — all vendored dependencies listed with version, license, and path.

---

## 5. Infrastructure & Polish ✓

### 5.1 Fix `http_basic.lua` example
- **Fixed:** Renamed to `examples/font_test.lua` with accurate name.

### 5.2 Rename `particles_readme.md` to `readme.md`
- **Fixed:** Renamed to `examples/particles/readme.md`.

### 5.3 Move `nuklear_demo.lua` into its own subdirectory
- **Fixed:** Moved to `examples/nuklear_demo/main.lua` with `readme.md`.

### 5.4 Add missing example READMEs
- **Created:** `examples/inheritance/readme.md`, `examples/microui/readme.md`, `examples/networking/readme.md`.

### 5.5 Fix `compile_flags.txt` missing include paths
- **Fixed:** Added `-Isrc`, `-Isrc/deps`, `-Isrc/deps/luasocket`, `-DNDEBUG`.

### 5.6 Add `.vscode/launch.json` and `.vscode/extensions.json`
- **Created:** Both files with recommended extensions and debug configurations.

### 5.7 Remove hard-coded local paths from `.vscode/settings.json`
- **Fixed:** Replaced hard-coded paths with environment variable references.

### 5.8 Add shell script equivalent of `setup-wrapper.ps1` for Android
- **Created:** `android/setup-wrapper.sh` for Linux/macOS developers.

### 5.9 Fix Android `abiFilters` inconsistency
- **Fixed:** Added `armeabi-v7a` to `abiFilters` in `app/build.gradle`.

### 5.10 Update readme LÖVE comparison
- **Fixed:** Updated to reflect Spry has gamepad, networking, HTTP, and threads.

### 5.11 Update website copyright year
- **Fixed:** Updated to `2023&ndash;2026` in `website/index.php`.
