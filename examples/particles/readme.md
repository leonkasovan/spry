# Particle Effects Example

A colorful interactive particle system demonstration with multiple spawning modes:

## Features

- **Interactive Spawning**: Click and drag to continuously spray particles
- **Burst Effects**: Right-click for radial particle bursts
- **Physics Simulation**: Gravity, velocity, and air resistance
- **Alpha Fading**: Particles fade out over their lifetime
- **Size Scaling**: Particles shrink as they age
- **Randomization**: Each particle has random velocity, color, size, and lifetime

## Controls

- **Left Click & Drag**: Spray particles upward with random spread
- **Right Click**: Create a blue burst effect
- **Space**: Explosion at screen center
- **F**: Spawn multiple fireworks
- **A**: Create fire effect with rising flames
- **C**: Clear all particles

## How to Run

```bash
spry examples/particles.lua
```

## Implementation Details

**Particle Properties:**
- Position (x, y)
- Velocity (vx, vy)
- Lifetime tracking (life, max_life)
- Size and color

**Physics:**
- Gravity: 300 pixels/second²
- Air resistance: velocity damping
- Frame-rate independent updates using delta time

**Spawning Modes:**
1. **Spray**: Random directional spread, mainly upward
2. **Burst**: Circular radial pattern with uniform angular distribution

**Rendering:**
- Alpha blending based on lifetime ratio
- Size reduction over time
- Color variation per particle
