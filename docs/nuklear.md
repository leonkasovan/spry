# spry.nuklear — Immediate-Mode GUI

An immediate-mode graphical user interface module powered by [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear). Build windows, buttons, sliders, text fields, menus, and more — entirely from Lua, every frame.

- **Immediate-mode**: UI is declared each frame inside `spry.frame()`. No persistent widget objects.
- **Automatic input**: Mouse, keyboard, and scroll events are forwarded automatically.
- **Rendering**: Uses the built-in microui bitmap font atlas; all drawing goes through sokol_gl.
- **Build option**: Controlled by `USE_NUKLEAR` CMake option (default ON). Set `-DUSE_NUKLEAR=OFF` to exclude.

> **Tip:** Combine window flags with Lua's bitwise OR operator (`|`).

---

## Quick Example

```lua
local nk = spry.nuklear

local slider = 50
local checked = false

function spry.frame(dt)
  local flags = nk.WINDOW_BORDER | nk.WINDOW_MOVABLE | nk.WINDOW_TITLE
  if nk.window_begin("My Window", 50, 50, 300, 200, flags) then
    nk.layout_row_dynamic(30, 1)
    nk.label("Hello from Nuklear!")

    nk.layout_row_dynamic(25, 1)
    slider = nk.slider_float(0, slider, 100, 1)

    nk.layout_row_dynamic(25, 1)
    checked = nk.checkbox_label("Enable", checked)

    nk.layout_row_dynamic(30, 1)
    if nk.button_label("Click Me") then
      print("clicked!")
    end
  end
  nk.window_end()
end
```

---

## Constants

### Window Flags

Combine with `|` and pass as the `flags` argument to `window_begin`.

| Constant                    | Description |
|-----------------------------|-------------|
| `WINDOW_BORDER`             | Draw a border around the window. |
| `WINDOW_MOVABLE`            | Window can be dragged by its header. |
| `WINDOW_SCALABLE`           | Window can be resized by dragging the lower-right corner. |
| `WINDOW_CLOSABLE`           | Window shows a close button in the header. |
| `WINDOW_MINIMIZABLE`        | Window shows a minimize button in the header. |
| `WINDOW_NO_SCROLLBAR`       | Hide the window scrollbar. |
| `WINDOW_TITLE`              | Display the window title in the header. |
| `WINDOW_SCROLL_AUTO_HIDE`   | Auto-hide the scrollbar when not needed. |
| `WINDOW_BACKGROUND`         | Always keep the window in the background. |
| `WINDOW_SCALE_LEFT`         | Allow scaling from the left side. |
| `WINDOW_NO_INPUT`           | Window does not receive input. |

### Text Alignment

| Constant         | Description |
|------------------|-------------|
| `TEXT_LEFT`      | Left-aligned text. |
| `TEXT_CENTERED`  | Center-aligned text. |
| `TEXT_RIGHT`     | Right-aligned text. |

### Layout Format

| Constant   | Description |
|------------|-------------|
| `DYNAMIC`  | Ratio-based layout (values 0.0–1.0). |
| `STATIC`   | Pixel-based layout (values in pixels). |

### Tree Type

| Constant     | Description |
|--------------|-------------|
| `TREE_NODE`  | Collapsible tree node. |
| `TREE_TAB`   | Collapsible tab-like node. |

### Collapse States

| Constant     | Description |
|--------------|-------------|
| `MINIMIZED`  | Node or window is collapsed. |
| `MAXIMIZED`  | Node or window is expanded. |

### Show States

| Constant | Description |
|----------|-------------|
| `HIDDEN` | Window is hidden. |
| `SHOWN`  | Window is visible. |

### Symbol Types

| Constant                  | Description |
|---------------------------|-------------|
| `SYMBOL_NONE`             | No symbol. |
| `SYMBOL_X`               | ✕ mark. |
| `SYMBOL_CIRCLE_SOLID`    | Filled circle. |
| `SYMBOL_CIRCLE_OUTLINE`  | Circle outline. |
| `SYMBOL_RECT_SOLID`      | Filled rectangle. |
| `SYMBOL_RECT_OUTLINE`    | Rectangle outline. |
| `SYMBOL_TRIANGLE_UP`     | Triangle pointing up. |
| `SYMBOL_TRIANGLE_DOWN`   | Triangle pointing down. |
| `SYMBOL_TRIANGLE_LEFT`   | Triangle pointing left. |
| `SYMBOL_TRIANGLE_RIGHT`  | Triangle pointing right. |
| `SYMBOL_PLUS`            | Plus sign. |
| `SYMBOL_MINUS`           | Minus sign. |

### Popup Type

| Constant        | Description |
|-----------------|-------------|
| `POPUP_STATIC`  | Static popup with fixed position. |
| `POPUP_DYNAMIC` | Dynamic popup that adjusts to content. |

### Chart Type

| Constant       | Description |
|----------------|-------------|
| `CHART_LINES`  | Line chart. |
| `CHART_COLUMN` | Column/bar chart. |

### Chart Events

Returned by `chart_push`.

| Constant         | Description |
|------------------|-------------|
| `CHART_HOVERING` | The pushed value is being hovered. |
| `CHART_CLICKED`  | The pushed value was clicked. |

### Color Format

| Constant | Description |
|----------|-------------|
| `RGB`    | Red, green, blue (no alpha). |
| `RGBA`   | Red, green, blue, alpha. |

### Edit Flags

Combine with `|` and pass to `edit_string`.

| Constant           | Description |
|--------------------|-------------|
| `EDIT_DEFAULT`     | Default edit behavior. |
| `EDIT_READ_ONLY`   | Text field is read-only. |
| `EDIT_AUTO_SELECT`  | Auto-select all text on activation. |
| `EDIT_SIG_ENTER`   | Signal when Enter is pressed. |
| `EDIT_ALLOW_TAB`   | Allow Tab character input. |
| `EDIT_NO_CURSOR`   | Hide the text cursor. |
| `EDIT_SELECTABLE`  | Allow text selection. |
| `EDIT_CLIPBOARD`   | Enable clipboard (Ctrl+C/V/X). |
| `EDIT_MULTILINE`   | Allow multiple lines. |

### Edit Types (presets)

| Constant       | Description |
|----------------|-------------|
| `EDIT_SIMPLE`  | Simple single-line edit. |
| `EDIT_FIELD`   | Standard text field. |
| `EDIT_BOX`     | Multiline text box. |
| `EDIT_EDITOR`  | Full editor with selection and clipboard. |

### Edit Events

Returned by `edit_string` as the first return value.

| Constant            | Description |
|---------------------|-------------|
| `EDIT_ACTIVE`       | Edit widget is currently active. |
| `EDIT_INACTIVE`     | Edit widget is inactive. |
| `EDIT_ACTIVATED`    | Edit widget was just activated this frame. |
| `EDIT_DEACTIVATED`  | Edit widget was just deactivated this frame. |
| `EDIT_COMMITED`     | Enter was pressed (with `EDIT_SIG_ENTER`). |

---

## API Reference

### Windows

#### `nk.window_begin(title, x, y, w, h [, flags])`

Begin a new window. Must be paired with `window_end()`. All widgets between `window_begin` and `window_end` belong to this window.

**Parameters**

| Name    | Type      | Description |
|---------|-----------|-------------|
| `title` | `string`  | Window title (also used as unique identifier). |
| `x`     | `number`  | Initial X position. |
| `y`     | `number`  | Initial Y position. |
| `w`     | `number`  | Initial width. |
| `h`     | `number`  | Initial height. |
| `flags` | `integer` | Optional. Bitwise OR of `WINDOW_*` constants. Default `0`. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the window is not collapsed/hidden and content should be drawn. |

---

#### `nk.window_end()`

End the current window. Must be called after `window_begin`, even when it returns `false`.

---

#### `nk.window_get_bounds()`

Get the current window's bounding rectangle.

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | X position. |
| 2 | `number` | Y position. |
| 3 | `number` | Width. |
| 4 | `number` | Height. |

---

#### `nk.window_get_size()`

Get the current window's size.

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | Width. |
| 2 | `number` | Height. |

---

#### `nk.window_get_position()`

Get the current window's position.

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | X position. |
| 2 | `number` | Y position. |

---

#### `nk.window_get_content_region()`

Get the usable content region of the current window (excludes header, borders, scrollbar).

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | X position. |
| 2 | `number` | Y position. |
| 3 | `number` | Width. |
| 4 | `number` | Height. |

---

#### `nk.window_has_focus()`

Check if the current window has input focus.

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if focused. |

---

#### `nk.window_is_hovered()`

Check if the current window is hovered by the mouse.

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if hovered. |

---

#### `nk.window_is_any_hovered()`

Check if any window is hovered by the mouse. Useful for deciding whether to pass input to the game.

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if any nuklear window is hovered. |

---

#### `nk.item_is_any_active()`

Check if any widget in any window is currently active (being interacted with).

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if any item is active. |

---

#### `nk.window_set_bounds(name, x, y, w, h)`

Set a window's position and size.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |
| `x`    | `number` | X position. |
| `y`    | `number` | Y position. |
| `w`    | `number` | Width. |
| `h`    | `number` | Height. |

---

#### `nk.window_set_position(name, x, y)`

Set a window's position.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |
| `x`    | `number` | X position. |
| `y`    | `number` | Y position. |

---

#### `nk.window_set_size(name, w, h)`

Set a window's size.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |
| `w`    | `number` | Width. |
| `h`    | `number` | Height. |

---

#### `nk.window_set_focus(name)`

Give input focus to a window.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |

---

#### `nk.window_close(name)`

Close a window. It will no longer be drawn.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |

---

#### `nk.window_collapse(name, state)`

Collapse or expand a window.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `name`  | `string`  | Window title/identifier. |
| `state` | `integer` | `MINIMIZED` or `MAXIMIZED`. |

---

#### `nk.window_show(name, state)`

Show or hide a window.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `name`  | `string`  | Window title/identifier. |
| `state` | `integer` | `HIDDEN` or `SHOWN`. |

---

#### `nk.window_is_collapsed(name)`

Check if a window is collapsed.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if collapsed. |

---

#### `nk.window_is_closed(name)`

Check if a window has been closed.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if closed. |

---

#### `nk.window_is_hidden(name)`

Check if a window is hidden.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if hidden. |

---

#### `nk.window_is_active(name)`

Check if a window is the currently active window.

| Name   | Type     | Description |
|--------|----------|-------------|
| `name` | `string` | Window title/identifier. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if active. |

---

### Layout

Layout functions control how widgets are positioned within a window. You must call a layout function before adding widgets.

#### `nk.layout_row_dynamic(height, cols)`

Create a dynamically sized row. Each column gets equal width that fills the window.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `height` | `number`  | Row height in pixels. |
| `cols`   | `integer` | Number of columns. |

**Example:**
```lua
nk.layout_row_dynamic(30, 2)   -- 2 equal-width columns, 30px tall
nk.button_label("Left")
nk.button_label("Right")
```

---

#### `nk.layout_row_static(height, item_width, cols)`

Create a statically sized row. Each column has fixed pixel width.

| Name         | Type      | Description |
|--------------|-----------|-------------|
| `height`     | `number`  | Row height in pixels. |
| `item_width` | `integer` | Width of each column in pixels. |
| `cols`       | `integer` | Number of columns. |

---

#### `nk.layout_row_begin(format, height, cols)`

Begin a custom row with per-column widths. Follow with `layout_row_push` for each column, then `layout_row_end`.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `format` | `integer` | `DYNAMIC` (ratio 0–1) or `STATIC` (pixels). |
| `height` | `number`  | Row height in pixels. |
| `cols`   | `integer` | Number of columns. |

**Example:**
```lua
nk.layout_row_begin(nk.STATIC, 30, 2)
nk.layout_row_push(100)    -- first column: 100px
nk.label("Name:")
nk.layout_row_push(200)    -- second column: 200px
_, text = nk.edit_string(nk.EDIT_FIELD, text, 128)
nk.layout_row_end()
```

---

#### `nk.layout_row_push(value)`

Set the width of the next column in a custom row.

| Name    | Type     | Description |
|---------|----------|-------------|
| `value` | `number` | Width in pixels (`STATIC`) or ratio 0–1 (`DYNAMIC`). |

---

#### `nk.layout_row_end()`

End a custom row started with `layout_row_begin`.

---

#### `nk.layout_space_begin(format, height, count)`

Begin an absolute/relative positioning layout. Widgets are placed with `layout_space_push`.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `format` | `integer` | `STATIC` (pixels) or `DYNAMIC` (ratios). |
| `height` | `number`  | Total height of the space. |
| `count`  | `integer` | Number of widgets. |

---

#### `nk.layout_space_push(x, y, w, h)`

Position a widget within a layout space.

| Name | Type     | Description |
|------|----------|-------------|
| `x`  | `number` | X offset. |
| `y`  | `number` | Y offset. |
| `w`  | `number` | Width. |
| `h`  | `number` | Height. |

---

#### `nk.layout_space_end()`

End a layout space.

---

#### `nk.spacer()`

Insert an empty space taking up one widget slot in the current row.

---

### Groups

Groups are scrollable sub-regions within a window.

#### `nk.group_begin(title [, flags])`

Begin a scrollable group. Must be paired with `group_end()`.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `title` | `string`  | Group identifier. |
| `flags` | `integer` | Optional. Bitwise OR of `WINDOW_*` constants. Default `0`. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the group is visible. |

---

#### `nk.group_end()`

End the current group. Only call when `group_begin` returned `true`.

---

### Trees

Collapsible tree nodes for hierarchical content.

#### `nk.tree_push(type, title [, state [, seed]])`

Push a collapsible tree node. Must be paired with `tree_pop()` when it returns `true`.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `type`  | `integer` | `TREE_NODE` or `TREE_TAB`. |
| `title` | `string`  | Node label (also used for internal hashing). |
| `state` | `integer` | Optional. Initial state: `MINIMIZED` (default) or `MAXIMIZED`. |
| `seed`  | `integer` | Optional. Hash seed for disambiguation when titles repeat. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the node is expanded and content should be drawn. |

**Example:**
```lua
if nk.tree_push(nk.TREE_TAB, "Settings", nk.MAXIMIZED) then
  nk.layout_row_dynamic(25, 1)
  nk.label("Volume:")
  nk.layout_row_dynamic(25, 1)
  volume = nk.slider_float(0, volume, 100, 1)
  nk.tree_pop()
end
```

---

#### `nk.tree_pop()`

Pop the current tree node. Only call when `tree_push` returned `true`.

---

### Labels & Text

#### `nk.label(text [, alignment])`

Display a text label.

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `text`      | `string`  | Text content. |
| `alignment` | `integer` | Optional. `TEXT_LEFT` (default), `TEXT_CENTERED`, or `TEXT_RIGHT`. |

---

#### `nk.label_colored(text, alignment, r, g, b [, a])`

Display a colored text label.

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `text`      | `string`  | Text content. |
| `alignment` | `integer` | `TEXT_LEFT`, `TEXT_CENTERED`, or `TEXT_RIGHT`. |
| `r`         | `integer` | Red (0–255). |
| `g`         | `integer` | Green (0–255). |
| `b`         | `integer` | Blue (0–255). |
| `a`         | `integer` | Optional. Alpha (0–255). Default `255`. |

---

#### `nk.label_wrap(text)`

Display a text label that wraps to fit the available width.

| Name   | Type     | Description |
|--------|----------|-------------|
| `text` | `string` | Text content. |

---

### Buttons

#### `nk.button_label(title)`

A clickable text button.

| Name    | Type     | Description |
|---------|----------|-------------|
| `title` | `string` | Button text. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if clicked this frame. |

---

#### `nk.button_color(r, g, b [, a])`

A clickable color swatch button.

| Name | Type      | Description |
|------|-----------|-------------|
| `r`  | `integer` | Red (0–255). |
| `g`  | `integer` | Green (0–255). |
| `b`  | `integer` | Blue (0–255). |
| `a`  | `integer` | Optional. Alpha (0–255). Default `255`. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if clicked this frame. |

---

#### `nk.button_symbol(symbol)`

A button displaying a symbol icon.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `symbol` | `integer` | One of the `SYMBOL_*` constants. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if clicked this frame. |

---

#### `nk.button_symbol_label(symbol, label [, alignment])`

A button with both a symbol icon and text.

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `symbol`    | `integer` | One of the `SYMBOL_*` constants. |
| `label`     | `string`  | Button text. |
| `alignment` | `integer` | Optional. Text alignment. Default `TEXT_LEFT`. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if clicked this frame. |

---

### Checkbox & Radio

#### `nk.checkbox_label(label, active)`

A toggle checkbox.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `label`  | `string`  | Checkbox text. |
| `active` | `boolean` | Current checked state. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | Updated checked state. |

**Example:**
```lua
nk.layout_row_dynamic(25, 1)
show_grid = nk.checkbox_label("Show Grid", show_grid)
```

---

#### `nk.option_label(label, active)`

A radio button. Only one in a group should be active.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `label`  | `string`  | Radio button text. |
| `active` | `boolean` | Whether this option is currently selected. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if this option was selected. |

**Example:**
```lua
nk.layout_row_dynamic(25, 3)
if nk.option_label("Easy",   difficulty == 1) then difficulty = 1 end
if nk.option_label("Normal", difficulty == 2) then difficulty = 2 end
if nk.option_label("Hard",   difficulty == 3) then difficulty = 3 end
```

---

### Selectable

#### `nk.selectable_label(label, alignment, value)`

A selectable text item (toggleable highlight).

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `label`     | `string`  | Item text. |
| `alignment` | `integer` | `TEXT_LEFT`, `TEXT_CENTERED`, or `TEXT_RIGHT`. |
| `value`     | `boolean` | Current selected state. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | Updated selected state. |

---

### Sliders

#### `nk.slider_float(min, value, max, step)`

A floating-point slider.

| Name    | Type     | Description |
|---------|----------|-------------|
| `min`   | `number` | Minimum value. |
| `value` | `number` | Current value. |
| `max`   | `number` | Maximum value. |
| `step`  | `number` | Step increment. |

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | Updated value. |

---

#### `nk.slider_int(min, value, max, step)`

An integer slider.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `min`   | `integer` | Minimum value. |
| `value` | `integer` | Current value. |
| `max`   | `integer` | Maximum value. |
| `step`  | `integer` | Step increment. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Updated value. |

---

### Progress

#### `nk.progress(current, max, modifiable)`

A progress bar.

| Name         | Type      | Description |
|--------------|-----------|-------------|
| `current`    | `integer` | Current progress value. |
| `max`        | `integer` | Maximum value. |
| `modifiable` | `boolean` | If `true`, the user can click to change the value. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Updated progress value. |

---

### Properties

Properties are labeled, draggable number inputs with increment/decrement buttons.

#### `nk.property_float(name, min, value, max, step, inc_per_pixel)`

A floating-point property widget.

| Name            | Type     | Description |
|-----------------|----------|-------------|
| `name`          | `string` | Label (prefix with `#` to show the label). |
| `min`           | `number` | Minimum value. |
| `value`         | `number` | Current value. |
| `max`           | `number` | Maximum value. |
| `step`          | `number` | Button step increment. |
| `inc_per_pixel` | `number` | Value change per pixel when dragging. |

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | Updated value. |

**Example:**
```lua
nk.layout_row_dynamic(25, 1)
speed = nk.property_float("#Speed:", 0, speed, 100, 1, 0.5)
```

---

#### `nk.property_int(name, min, value, max, step, inc_per_pixel)`

An integer property widget.

| Name            | Type      | Description |
|-----------------|-----------|-------------|
| `name`          | `string`  | Label (prefix with `#` to show the label). |
| `min`           | `integer` | Minimum value. |
| `value`         | `integer` | Current value. |
| `max`           | `integer` | Maximum value. |
| `step`          | `integer` | Button step increment. |
| `inc_per_pixel` | `number`  | Value change per pixel when dragging. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Updated value. |

---

### Text Edit

#### `nk.edit_string(flags, text, max_length)`

An editable text field.

| Name         | Type      | Description |
|--------------|-----------|-------------|
| `flags`      | `integer` | Bitwise OR of `EDIT_*` flags, or a preset like `EDIT_FIELD`. |
| `text`       | `string`  | Current text content. |
| `max_length` | `integer` | Optional. Maximum buffer size. Default `256`. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Bitwise OR of `EDIT_*` event flags (e.g. `EDIT_ACTIVE`, `EDIT_COMMITED`). |
| 2 | `string`  | Updated text content. |

**Example:**
```lua
nk.layout_row_dynamic(25, 1)
local events, name = nk.edit_string(nk.EDIT_FIELD, name, 64)
if events & nk.EDIT_COMMITED ~= 0 then
  print("Submitted: " .. name)
end
```

---

### Color Picker

#### `nk.color_picker(r, g, b [, a [, format]])`

An interactive color picker widget.

| Name     | Type      | Description |
|----------|-----------|-------------|
| `r`      | `number`  | Red component (0.0–1.0). |
| `g`      | `number`  | Green component (0.0–1.0). |
| `b`      | `number`  | Blue component (0.0–1.0). |
| `a`      | `number`  | Optional. Alpha component (0.0–1.0). Default `1.0`. |
| `format` | `integer` | Optional. `RGB` or `RGBA`. Default `RGBA`. |

**Returns**

| # | Type     | Description |
|---|----------|-------------|
| 1 | `number` | Updated red. |
| 2 | `number` | Updated green. |
| 3 | `number` | Updated blue. |
| 4 | `number` | Updated alpha. |

---

### Combo Box

#### `nk.combo(items, selected, item_height, width, height)`

A dropdown combo box.

| Name          | Type      | Description |
|---------------|-----------|-------------|
| `items`       | `table`   | Array of strings (1-indexed). |
| `selected`    | `integer` | Currently selected index (1-indexed). |
| `item_height` | `integer` | Height of each item in the dropdown. |
| `width`       | `number`  | Dropdown popup width. |
| `height`      | `number`  | Dropdown popup max height. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Updated selected index (1-indexed). |

**Example:**
```lua
local items = {"Apple", "Banana", "Cherry"}
nk.layout_row_dynamic(25, 1)
selected = nk.combo(items, selected, 25, 200, 150)
```

---

### Popups

Modal/non-modal overlay windows anchored to the current window.

#### `nk.popup_begin(type, title, flags, x, y, w, h)`

Begin a popup. Must be paired with `popup_end()`.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `type`  | `integer` | `POPUP_STATIC` or `POPUP_DYNAMIC`. |
| `title` | `string`  | Popup identifier. |
| `flags` | `integer` | Bitwise OR of `WINDOW_*` constants. |
| `x`     | `number`  | X offset relative to parent window. |
| `y`     | `number`  | Y offset relative to parent window. |
| `w`     | `number`  | Width. |
| `h`     | `number`  | Height. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the popup is open. |

---

#### `nk.popup_close()`

Close the current popup.

---

#### `nk.popup_end()`

End the current popup. Only call when `popup_begin` returned `true`.

---

### Contextual Menus

Right-click context menus.

#### `nk.contextual_begin(flags, w, h, trigger_x, trigger_y, trigger_w, trigger_h)`

Begin a contextual menu. Must be paired with `contextual_end()`.

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `flags`     | `integer` | Bitwise OR of `WINDOW_*` constants. |
| `w`         | `number`  | Menu popup width. |
| `h`         | `number`  | Menu popup height. |
| `trigger_x` | `number`  | Trigger area X. |
| `trigger_y` | `number`  | Trigger area Y. |
| `trigger_w` | `number`  | Trigger area width. |
| `trigger_h` | `number`  | Trigger area height. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the contextual menu is open. |

---

#### `nk.contextual_item_label(label [, alignment])`

Add a clickable item to the contextual menu.

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `label`     | `string`  | Item text. |
| `alignment` | `integer` | Optional. Text alignment. Default `TEXT_LEFT`. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if clicked. |

---

#### `nk.contextual_close()`

Close the current contextual menu.

---

#### `nk.contextual_end()`

End the current contextual menu. Only call when `contextual_begin` returned `true`.

---

### Tooltips

#### `nk.tooltip(text)`

Display a simple text tooltip at the mouse cursor position.

| Name   | Type     | Description |
|--------|----------|-------------|
| `text` | `string` | Tooltip text. |

---

#### `nk.tooltip_begin(width)`

Begin a complex tooltip with arbitrary widget content. Must be paired with `tooltip_end()`.

| Name    | Type     | Description |
|---------|----------|-------------|
| `width` | `number` | Tooltip width. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the tooltip is active. |

---

#### `nk.tooltip_end()`

End a complex tooltip.

---

### Menubar

A horizontal menu bar at the top of a window.

#### `nk.menubar_begin()`

Begin the menubar region. Call this right after `window_begin`.

---

#### `nk.menubar_end()`

End the menubar region.

---

#### `nk.menu_begin_label(label, alignment, w, h)`

Begin a dropdown menu in the menubar. Must be paired with `menu_end()`.

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `label`     | `string`  | Menu title. |
| `alignment` | `integer` | `TEXT_LEFT`, `TEXT_CENTERED`, or `TEXT_RIGHT`. |
| `w`         | `number`  | Dropdown width. |
| `h`         | `number`  | Dropdown height. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the menu is open. |

---

#### `nk.menu_item_label(label [, alignment])`

Add a clickable item to the current menu.

| Name        | Type      | Description |
|-------------|-----------|-------------|
| `label`     | `string`  | Item text. |
| `alignment` | `integer` | Optional. Text alignment. Default `TEXT_LEFT`. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if clicked. |

---

#### `nk.menu_close()`

Close the current menu.

---

#### `nk.menu_end()`

End the current menu. Only call when `menu_begin_label` returned `true`.

**Example:**
```lua
nk.menubar_begin()
nk.layout_row_begin(nk.STATIC, 25, 2)

nk.layout_row_push(45)
if nk.menu_begin_label("File", nk.TEXT_LEFT, 120, 200) then
  nk.layout_row_dynamic(25, 1)
  if nk.menu_item_label("New")  then print("New")  end
  if nk.menu_item_label("Open") then print("Open") end
  if nk.menu_item_label("Save") then print("Save") end
  nk.menu_end()
end

nk.layout_row_push(45)
if nk.menu_begin_label("Edit", nk.TEXT_LEFT, 120, 200) then
  nk.layout_row_dynamic(25, 1)
  if nk.menu_item_label("Undo") then print("Undo") end
  if nk.menu_item_label("Redo") then print("Redo") end
  nk.menu_end()
end

nk.layout_row_end()
nk.menubar_end()
```

---

### Charts

Simple line and column charts.

#### `nk.chart_begin(type, count, min, max)`

Begin a chart widget. Push values with `chart_push`, then call `chart_end`.

| Name    | Type      | Description |
|---------|-----------|-------------|
| `type`  | `integer` | `CHART_LINES` or `CHART_COLUMN`. |
| `count` | `integer` | Number of data points. |
| `min`   | `number`  | Minimum Y value. |
| `max`   | `number`  | Maximum Y value. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `boolean` | `true` if the chart is visible. |

---

#### `nk.chart_push(value)`

Push a data point to the current chart.

| Name    | Type     | Description |
|---------|----------|-------------|
| `value` | `number` | Data point value. |

**Returns**

| # | Type      | Description |
|---|-----------|-------------|
| 1 | `integer` | Bitwise OR of chart event flags (`CHART_HOVERING`, `CHART_CLICKED`), or `0`. |

---

#### `nk.chart_end()`

End the current chart.

**Example:**
```lua
nk.layout_row_dynamic(100, 1)
if nk.chart_begin(nk.CHART_LINES, 10, 0, 100) then
  for i = 1, 10 do
    local v = 50 + math.sin(spry.elapsed() + i) * 40
    nk.chart_push(v)
  end
  nk.chart_end()
end
```

---

### Miscellaneous

#### `nk.rule_horizontal(r, g, b [, a [, rounding]])`

Draw a horizontal separator line.

| Name       | Type      | Description |
|------------|-----------|-------------|
| `r`        | `integer` | Red (0–255). |
| `g`        | `integer` | Green (0–255). |
| `b`        | `integer` | Blue (0–255). |
| `a`        | `integer` | Optional. Alpha (0–255). Default `255`. |
| `rounding` | `boolean` | Optional. Apply rounding to the line. Default `false`. |

---

## Input Handling Tips

Nuklear consumes mouse and keyboard input. To prevent your game from reacting to clicks on the UI:

```lua
function spry.frame(dt)
  -- Draw UI first
  if nk.window_begin("Panel", 10, 10, 200, 100, flags) then
    -- ...widgets...
  end
  nk.window_end()

  -- Check if nuklear wants input before handling game input
  if not nk.window_is_any_hovered() and not nk.item_is_any_active() then
    -- Safe to process game mouse clicks here
  end
end
```

---

## Rendering Notes

- Circles are approximated as filled rectangles (no polygon subdivision).
- Triangles are rendered as actual triangles via `sgl_begin_triangles`.
- Image commands (`NK_COMMAND_IMAGE`) are not currently supported.
- The built-in font is a small bitmap font from the microui atlas. It is not scalable.
