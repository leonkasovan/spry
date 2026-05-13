# spry.tilemap — LDtk Level Loading

Spry can load [LDtk](https://ldtk.io/) level files (`.ldtk`) directly — no
export step needed. Tilemaps include layers, entities, collision grids, and
A* pathfinding.

## Loading

```lua
tilemap = spry.tilemap_load("level.ldtk")
```

## Rendering

### `tilemap:draw([world_x, world_y])`

Draw all visible layers at the given world offset (for camera scrolling).

```lua
function spry.frame(dt)
  tilemap:draw(camera_x, camera_y)
end
```

### `tilemap:draw_fixtures()`

Debug-draw collision fixtures as wireframes.

---

## Entities

### `tilemap:entities([layer_name])`

Return a table of entities defined in the LDtk level. Optionally filter by
layer name. Each entity has the fields defined in LDtk.

```lua
local entities = tilemap:entities("entities")
for _, e in ipairs(entities) do
  print(e.x, e.y, e.width, e.height, e.name)
end
```

---

## Collision

### `tilemap:make_collision(world, [layer_name])`

Create Box2D static bodies for all solid tiles in an `IntGrid` layer.
Returns the created bodies.

```lua
local world = spry.b2_world(0, 980)
tilemap:make_collision(world, "solid")
```

The layer name defaults to `"collision"` if not specified.

---

## Pathfinding

### `tilemap:make_graph([layer_name])`

Build an A* navigation graph from an `IntGrid` layer. Walkable tiles are
those with cost > 0.

```lua
local graph = tilemap:make_graph("ground")
```

### `tilemap:astar(graph, x1, y1, x2, y2)`

Find a path from `(x1, y1)` to `(x2, y2)` using the navigation graph.
Coordinates are in tile coordinates (not pixels).

```lua
local path = tilemap:astar(graph, 5, 10, 20, 15)
if path then
  for _, node in ipairs(path) do
    print(node.x, node.y)
  end
end
```

Returns an array of `{x, y}` tile coordinates, or `nil` if no path exists.

---

## See Also

- `examples/dungeon/` — Dungeon game with LDtk, collision, and pathfinding
- `examples/jump/` — Platformer with LDtk and tile collision
- `examples/pathfinding/` — A* pathfinding demonstration
