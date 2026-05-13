# spry.microui — Immediate-Mode GUI

A binding of the [microui](https://github.com/rxi/microui) library — a
minimal immediate-mode GUI toolkit.

## Getting Started

```lua
local mu = spry.microui

function spry.start()
  font = spry.default_font()
end

function spry.frame(dt)
  if mu.begin_window("Hello", mu.rect(10, 10, 300, 200)) then
    mu.layout_row({-1}, 0)
    mu.label "Hello, World!"
    if mu.button "Click Me" then
      print("clicked!")
    end
    mu.end_window()
  end
end
```

---

## Window Functions

| Function | Description |
|----------|-------------|
| `mu.begin_window(title, rect)` | Start a window. Returns `true` if the window is open. |
| `mu.end_window()` | End the current window. |
| `mu.begin_panel(name, rect)` | Start a scrollable panel (inside a window). |
| `mu.end_panel()` | End the current panel. |
| `mu.begin_popup(name)` | Start a popup. Must follow a nearby `mu.button`. |
| `mu.end_popup()` | End the current popup. |
| `mu.open_popup(name)` | Open a named popup. |
| `mu.get_current_container()` | Returns the current window/panel container. |

### Container Methods

| Method | Description |
|--------|-------------|
| `container:rect()` | Get `{x, y, w, h}` rect. |
| `container:set_rect(t)` | Set `{x, y, w, h}` rect. |
| `container:body()` | Get body area `{x, y, w, h}`. |
| `container:scroll()` | Get `sx, sy` scroll position. |
| `container:set_scroll(x, y)` | Set scroll position. |
| `container:content_size()` | Get content `width, height`. |

---

## Layout

| Function | Description |
|----------|-------------|
| `mu.layout_row(widths, height)` | Set layout row with array of column widths (-1 = fill). |
| `mu.layout_next()` | Get the rect of the next widget. |
| `mu.layout_begin_column()` | Begin a sub-column. |
| `mu.layout_end_column()` | End a sub-column. |

---

## Widgets

| Function | Returns | Description |
|----------|---------|-------------|
| `mu.button(label)` | `boolean` | Push button. |
| `mu.label(text)` | — | Static label. |
| `mu.text(text)` | — | Word-wrapped text. |
| `mu.textbox(ref, [w])` | `flags` | Single-line text input. Returns `mu.RES_SUBMIT` on enter. |
| `mu.slider(ref, lo, hi [, step, fmt])` | — | Horizontal slider. `ref` is a `mu.ref()`. |
| `mu.checkbox(label, ref)` | `boolean` | Checkbox. `ref` is a `mu.ref()`. |
| `mu.header(label [, opt])` | `boolean` | Collapsible header (tree node). Pass `mu.OPT_EXPANDED` to start open. |
| `mu.begin_treenode(label)` | `boolean` | Tree node. Must call `mu.end_treenode()`. |
| `mu.end_treenode()` | — | End a tree node. |
| `mu.rect(w, h)` | `rect` | Draw a colored rect. |

---

## Drawing and Style

| Function | Description |
|----------|-------------|
| `mu.draw_rect(rect, color)` | Draw a filled rectangle. |
| `mu.draw_control_text(text, rect, color, opt)` | Draw text in a rect area. |
| `mu.get_style()` | Returns the style table. |
| `mu.set_focus(id)` | Set keyboard focus to a widget id. |
| `mu.get_last_id()` | Get the id of the last widget. |

### Style Methods

| Method | Description |
|--------|-------------|
| `style:set_color(idx, color)` | Set a theme color. Index is one of `mu.COLOR_*`. |

### Color Constants

`mu.COLOR_TEXT`, `mu.COLOR_BORDER`, `mu.COLOR_WINDOWBG`, `mu.COLOR_TITLEBG`,
`mu.COLOR_TITLETEXT`, `mu.COLOR_PANELBG`, `mu.COLOR_BUTTON`,
`mu.COLOR_BUTTONHOVER`, `mu.COLOR_BUTTONFOCUS`, `mu.COLOR_BASE`,
`mu.COLOR_BASEHOVER`, `mu.COLOR_BASEFOCUS`, `mu.COLOR_SCROLLBASE`,
`mu.COLOR_SCROLLTHUMB`.

---

## Ref System

`mu.ref(value)` creates a reference to a value that can be mutated across
frames (needed for widgets like sliders and textboxes).

```lua
local my_val = mu.ref(50)

-- In frame:
mu.slider(my_val, 0, 100)

-- Read:
local v = my_val:get()  -- 50 (or whatever the slider set it to)

-- Write:
my_val:set(75)
```

`mu.ref ""` creates a string reference for text input widgets.

---

## Return Flags

| Flag | Description |
|------|-------------|
| `mu.RES_SUBMIT` | The widget was submitted (e.g., enter in textbox). |
| `mu.OPT_EXPANDED` | Tree node or header expanded (used with `mu.header()`). |

---

## See Also

- `examples/microui/` — Full demo with windows, sliders, style editor, log
