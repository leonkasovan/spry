# Particle Maker Example

A lightweight particle system editor-inspired demo, ported from a Love2D script. It demonstrates a configurable emitter, random presets, and saving/loading settings.

## Controls

- Left mouse: move the emitter
- F1: toggle settings overlay
- F2: save current settings (writes `NAME.lua`)
- F3 or Enter: randomize settings
- PageUp/PageDown: load next/previous preset
- Esc: quit

## Assets

This example loads sprite images from the example folder. Add at least one PNG (for example `particle.png`) and update the `image_names` list in `main.lua` if needed.

## Presets

- Presets are regular Lua files that return a table, e.g. `default.lua`.
- Use F2 to save a new preset file in the same folder.

## How to Run

```bash
spry examples/particle_maker
```
