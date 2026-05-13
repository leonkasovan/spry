-- Nuklear UI Demo
-- Demonstrates the Nuklear immediate-mode GUI module

local nk = spry.nuklear

local slider_val = 50
local check_val = false
local option_val = 1
local combo_val = 1
local progress_val = 40
local prop_val = 10.0
local edit_text = "Hello"
local color_r, color_g, color_b, color_a = 1.0, 0.5, 0.0, 1.0

local items = {"Apple", "Banana", "Cherry", "Date", "Elderberry"}

function spry.conf(t)
  t.window_title = "Nuklear Demo"
  t.window_width = 900
  t.window_height = 700
end

function spry.start()
  font = spry.default_font()
end

function spry.frame(dt)
  spry.clear_color(40, 40, 50, 255)

  local flags = nk.WINDOW_BORDER | nk.WINDOW_MOVABLE | nk.WINDOW_SCALABLE
               | nk.WINDOW_MINIMIZABLE | nk.WINDOW_TITLE

  -- Main demo window
  if nk.window_begin("Nuklear Demo", 20, 20, 400, 650, flags) then

    -- Label
    nk.layout_row_dynamic(30, 1)
    nk.label("Welcome to Nuklear UI!")

    nk.layout_row_dynamic(20, 1)
    nk.label("FPS: " .. math.floor(1 / dt), nk.TEXT_RIGHT)

    -- Separator
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    -- Buttons
    nk.layout_row_dynamic(30, 1)
    nk.label("Buttons:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(30, 3)
    if nk.button_label("Click") then
      print("Button clicked!")
    end
    if nk.button_label("Press") then
      print("Press!")
    end
    if nk.button_color(
      math.floor(color_r * 255),
      math.floor(color_g * 255),
      math.floor(color_b * 255),
      255
    ) then
      print("Color button!")
    end

    -- Checkbox
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    nk.layout_row_dynamic(30, 1)
    nk.label("Checkbox:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(25, 1)
    check_val = nk.checkbox_label("Enable feature", check_val)

    if check_val then
      nk.layout_row_dynamic(20, 1)
      nk.label_colored("Feature is ON", nk.TEXT_LEFT, 100, 255, 100, 255)
    end

    -- Radio / Option
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    nk.layout_row_dynamic(30, 1)
    nk.label("Radio Buttons:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(25, 3)
    if nk.option_label("Easy", option_val == 1) then option_val = 1 end
    if nk.option_label("Normal", option_val == 2) then option_val = 2 end
    if nk.option_label("Hard", option_val == 3) then option_val = 3 end

    -- Slider
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    nk.layout_row_dynamic(30, 1)
    nk.label("Slider:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(25, 1)
    slider_val = nk.slider_float(0, slider_val, 100, 0.5)

    nk.layout_row_dynamic(20, 1)
    nk.label(string.format("Value: %.1f", slider_val), nk.TEXT_CENTERED)

    -- Progress
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    nk.layout_row_dynamic(30, 1)
    nk.label("Progress:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(25, 1)
    progress_val = nk.progress(progress_val, 100, true)

    nk.layout_row_dynamic(20, 1)
    nk.label(progress_val .. "%", nk.TEXT_CENTERED)

    -- Property
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    nk.layout_row_dynamic(30, 1)
    nk.label("Property:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(25, 1)
    prop_val = nk.property_float("#Speed:", 0, prop_val, 100, 1, 0.5)

    -- Combo
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    nk.layout_row_dynamic(30, 1)
    nk.label("Combo:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(25, 1)
    combo_val = nk.combo(items, combo_val, 25, 200, 150)

    nk.layout_row_dynamic(20, 1)
    nk.label("Selected: " .. items[combo_val], nk.TEXT_LEFT)

    -- Edit
    nk.layout_row_dynamic(10, 1)
    nk.rule_horizontal(100, 100, 100, 255, false)

    nk.layout_row_dynamic(30, 1)
    nk.label("Text Edit:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(25, 1)
    local flags_result
    flags_result, edit_text = nk.edit_string(nk.EDIT_FIELD, edit_text, 128)

    nk.layout_row_dynamic(20, 1)
    nk.label("Text: " .. edit_text, nk.TEXT_LEFT)
  end
  nk.window_end()

  -- Second window: Color Picker
  local flags2 = nk.WINDOW_BORDER | nk.WINDOW_MOVABLE | nk.WINDOW_TITLE
                | nk.WINDOW_MINIMIZABLE

  if nk.window_begin("Color Picker", 450, 20, 300, 350, flags2) then
    nk.layout_row_dynamic(20, 1)
    nk.label("Pick a color:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(180, 1)
    color_r, color_g, color_b, color_a =
      nk.color_picker(color_r, color_g, color_b, color_a, nk.RGBA)

    nk.layout_row_dynamic(20, 1)
    nk.label(string.format(
      "R:%.2f G:%.2f B:%.2f A:%.2f",
      color_r, color_g, color_b, color_a
    ), nk.TEXT_LEFT)

    -- Draw preview rect using spry drawing
    nk.layout_row_dynamic(30, 1)
    nk.label("Preview (drawn below window):", nk.TEXT_LEFT)
  end
  nk.window_end()

  -- Third window: Chart
  local flags3 = nk.WINDOW_BORDER | nk.WINDOW_MOVABLE | nk.WINDOW_TITLE

  if nk.window_begin("Chart", 450, 400, 300, 250, flags3) then
    nk.layout_row_dynamic(25, 1)
    nk.label("Line Chart:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(100, 1)
    if nk.chart_begin(nk.CHART_LINES, 10, 0, 100) then
      for i = 1, 10 do
        local v = 50 + math.sin((spry.elapsed() + i) * 0.5) * 40
        nk.chart_push(v)
      end
      nk.chart_end()
    end

    nk.layout_row_dynamic(25, 1)
    nk.label("Column Chart:", nk.TEXT_LEFT)

    nk.layout_row_dynamic(80, 1)
    if nk.chart_begin(nk.CHART_COLUMN, 5, 0, 100) then
      for i = 1, 5 do
        nk.chart_push(20 * i)
      end
      nk.chart_end()
    end
  end
  nk.window_end()

  -- Draw color preview below the color picker window
  spry.push_color(
    math.floor(color_r * 255),
    math.floor(color_g * 255),
    math.floor(color_b * 255),
    math.floor(color_a * 255)
  )
  spry.draw_filled_rect(780, 30, 80, 80)
  spry.pop_color()
end
