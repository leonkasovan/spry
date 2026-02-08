#include "gamepad.h"
#include "strings.h"
#include <math.h>
#include <string.h>

// ============================================================================
// Common helpers
// ============================================================================

float gamepad_apply_deadzone(float value, float deadzone) {
  if (fabsf(value) < deadzone) {
    return 0.0f;
  }
  float sign = value > 0.0f ? 1.0f : -1.0f;
  return sign * (fabsf(value) - deadzone) / (1.0f - deadzone);
}

i32 gamepad_count(GamepadState *state) {
  i32 n = 0;
  for (i32 i = 0; i < MAX_JOYSTICKS; i++) {
    if (state->joysticks[i].connected) {
      n++;
    }
  }
  return n;
}

void gamepad_end_frame(GamepadState *state) {
  for (i32 i = 0; i < MAX_JOYSTICKS; i++) {
    Joystick *j = &state->joysticks[i];
    memcpy(j->prev_buttons, j->buttons, sizeof(j->buttons));
  }
}

// ============================================================================
// SDL2-compatible mapping database parser
// ============================================================================

i32 gamepad_find_mapping(GamepadState *state, const char *guid) {
  for (u64 i = 0; i < state->mappings.len; i++) {
    if (strcmp(state->mappings[i].guid, guid) == 0) {
      return (i32)i;
    }
  }
  return -1;
}

// Skip whitespace.
static const char *skip_ws(const char *p) {
  while (*p == ' ' || *p == '\t') {
    p++;
  }
  return p;
}

// Read until delimiter, return pointer past it. Writes to buf (max n-1 chars).
static const char *read_field(const char *p, char *buf, u64 n, char delim) {
  u64 i = 0;
  while (*p && *p != delim && *p != '\n' && *p != '\r') {
    if (i < n - 1) {
      buf[i++] = *p;
    }
    p++;
  }
  buf[i] = 0;
  if (*p == delim) {
    p++;
  }
  return p;
}

// Map SDL2 target name -> our enum.
static i32 mapping_target_button(const char *name) {
  if (strcmp(name, "a") == 0) return GAMEPAD_BUTTON_A;
  if (strcmp(name, "b") == 0) return GAMEPAD_BUTTON_B;
  if (strcmp(name, "x") == 0) return GAMEPAD_BUTTON_X;
  if (strcmp(name, "y") == 0) return GAMEPAD_BUTTON_Y;
  if (strcmp(name, "back") == 0) return GAMEPAD_BUTTON_BACK;
  if (strcmp(name, "guide") == 0) return GAMEPAD_BUTTON_GUIDE;
  if (strcmp(name, "start") == 0) return GAMEPAD_BUTTON_START;
  if (strcmp(name, "leftstick") == 0) return GAMEPAD_BUTTON_LEFTSTICK;
  if (strcmp(name, "rightstick") == 0) return GAMEPAD_BUTTON_RIGHTSTICK;
  if (strcmp(name, "leftshoulder") == 0) return GAMEPAD_BUTTON_LEFTSHOULDER;
  if (strcmp(name, "rightshoulder") == 0) return GAMEPAD_BUTTON_RIGHTSHOULDER;
  if (strcmp(name, "dpup") == 0) return GAMEPAD_BUTTON_DPUP;
  if (strcmp(name, "dpdown") == 0) return GAMEPAD_BUTTON_DPDOWN;
  if (strcmp(name, "dpleft") == 0) return GAMEPAD_BUTTON_DPLEFT;
  if (strcmp(name, "dpright") == 0) return GAMEPAD_BUTTON_DPRIGHT;
  return -1;
}

static i32 mapping_target_axis(const char *name) {
  if (strcmp(name, "leftx") == 0) return GAMEPAD_AXIS_LEFTX;
  if (strcmp(name, "lefty") == 0) return GAMEPAD_AXIS_LEFTY;
  if (strcmp(name, "rightx") == 0) return GAMEPAD_AXIS_RIGHTX;
  if (strcmp(name, "righty") == 0) return GAMEPAD_AXIS_RIGHTY;
  if (strcmp(name, "lefttrigger") == 0) return GAMEPAD_AXIS_LEFT_TRIGGER;
  if (strcmp(name, "righttrigger") == 0) return GAMEPAD_AXIS_RIGHT_TRIGGER;
  return -1;
}

// Parse a single SDL2 mapping line.
static bool parse_mapping_line(const char *line, GamepadMapping *out) {
  memset(out, 0, sizeof(*out));
  for (i32 i = 0; i < GAMEPAD_BUTTON_MAX; i++) {
    out->button_bind[i] = -1;
    out->button_axis[i] = -1;
    out->button_hat[i] = -1;
  }
  for (i32 i = 0; i < GAMEPAD_AXIS_MAX; i++) {
    out->axis_bind[i] = -1;
    out->axis_button_pos[i] = -1;
    out->axis_button_neg[i] = -1;
  }

  const char *p = skip_ws(line);
  if (*p == '#' || *p == '\n' || *p == '\r' || *p == 0) {
    return false; // comment or empty
  }

  // GUID
  p = read_field(p, out->guid, sizeof(out->guid), ',');
  if (out->guid[0] == 0) {
    return false;
  }

  // Name
  p = read_field(p, out->name, sizeof(out->name), ',');

  // Bindings: "target:source,target:source,..."
  while (*p && *p != '\n' && *p != '\r') {
    p = skip_ws(p);
    if (*p == 0 || *p == '\n' || *p == '\r') {
      break;
    }

    char target[32] = {};
    char source[32] = {};

    p = read_field(p, target, sizeof(target), ':');
    p = read_field(p, source, sizeof(source), ',');

    if (target[0] == 0 || source[0] == 0) {
      continue;
    }

    // Skip platform hint.
    if (strcmp(target, "platform") == 0) {
      continue;
    }

    i32 btn_idx = mapping_target_button(target);
    i32 axis_idx = mapping_target_axis(target);

    // Parse source: b0, a0, h0.1, +a0, -a0, ~a0
    if (btn_idx >= 0) {
      if (source[0] == 'b') {
        // button -> button
        out->button_bind[btn_idx] = atoi(&source[1]);
      } else if (source[0] == 'a' || source[0] == '+' || source[0] == '-') {
        // axis -> button
        i32 dir = 0;
        const char *num = &source[1];
        if (source[0] == '+') {
          dir = 1;
          num = &source[2];
        } else if (source[0] == '-') {
          dir = -1;
          num = &source[2];
        } else {
          dir = 1; // default positive
        }
        out->button_axis[btn_idx] = atoi(num);
        out->button_axis_dir[btn_idx] = dir;
      } else if (source[0] == 'h') {
        // hat -> button: h0.1, h0.2, h0.4, h0.8
        const char *dot = strchr(source, '.');
        if (dot) {
          out->button_hat[btn_idx] = atoi(&source[1]);
          out->button_hat_mask[btn_idx] = atoi(dot + 1);
        }
      }
    } else if (axis_idx >= 0) {
      bool inverted = false;
      const char *s = source;
      if (s[0] == '~') {
        inverted = true;
        s++;
      }
      if (s[0] == 'a') {
        out->axis_bind[axis_idx] = atoi(&s[1]);
        out->axis_inverted[axis_idx] = inverted;
      } else if (s[0] == 'b') {
        // button -> axis (rare, e.g. dpad as axis)
        // We store positive direction button.
        out->axis_button_pos[axis_idx] = atoi(&s[1]);
      }
    }
  }

  return true;
}

i32 gamepad_add_mappings_from_string(GamepadState *state, String contents) {
  i32 added = 0;

  const char *p = contents.data;
  const char *end = contents.data + contents.len;

  while (p < end) {
    // Find end of line.
    const char *line_start = p;
    while (p < end && *p != '\n' && *p != '\r') {
      p++;
    }

    u64 line_len = (u64)(p - line_start);

    // Skip line ending.
    if (p < end && *p == '\r') {
      p++;
    }
    if (p < end && *p == '\n') {
      p++;
    }

    if (line_len == 0) {
      continue;
    }

    // Null-terminate a copy for parsing.
    char line_buf[1024];
    u64 copy_len = line_len < sizeof(line_buf) - 1 ? line_len : sizeof(line_buf) - 1;
    memcpy(line_buf, line_start, copy_len);
    line_buf[copy_len] = 0;

    GamepadMapping mapping = {};
    if (parse_mapping_line(line_buf, &mapping)) {
      // Replace existing mapping with same GUID, or add new.
      i32 existing = gamepad_find_mapping(state, mapping.guid);
      if (existing >= 0) {
        state->mappings[existing] = mapping;
      } else {
        state->mappings.push(mapping);
      }
      added++;
    }
  }

  return added;
}

// Apply mapping to translate raw state into standard gamepad state.
static void apply_mapping(Joystick *j, GamepadMapping *m, float deadzone) {
  // Clear standard state (buttons will be written, axes zeroed).
  for (i32 i = 0; i < GAMEPAD_AXIS_MAX; i++) {
    j->axes[i] = 0.0f;
  }

  // Buttons.
  for (i32 i = 0; i < GAMEPAD_BUTTON_MAX; i++) {
    bool pressed = false;

    if (m->button_bind[i] >= 0 && m->button_bind[i] < j->raw_button_count) {
      pressed = j->raw_buttons[m->button_bind[i]];
    }

    if (!pressed && m->button_axis[i] >= 0 &&
        m->button_axis[i] < j->raw_axis_count) {
      float val = j->raw_axes[m->button_axis[i]];
      if (m->button_axis_dir[i] > 0) {
        pressed = val > 0.5f;
      } else {
        pressed = val < -0.5f;
      }
    }

    // Hat support: on Linux, hats are reported as axes.
    // We treat hat axis 0 as X (left=-1 right=+1) and hat axis 1 as Y
    // (up=-1 down=+1). SDL hat bitmask: 1=up, 2=right, 4=down, 8=left.
    if (!pressed && m->button_hat[i] >= 0) {
      i32 hat_base = m->button_hat[i] * 2; // Two axes per hat.
      i32 mask = m->button_hat_mask[i];
      if (hat_base + 1 < j->raw_axis_count) {
        float hx = j->raw_axes[hat_base];
        float hy = j->raw_axes[hat_base + 1];
        if ((mask & 1) && hy < -0.5f) pressed = true; // up
        if ((mask & 2) && hx > 0.5f) pressed = true;  // right
        if ((mask & 4) && hy > 0.5f) pressed = true;   // down
        if ((mask & 8) && hx < -0.5f) pressed = true;  // left
      }
    }

    j->buttons[i] = pressed;
  }

  // Axes.
  for (i32 i = 0; i < GAMEPAD_AXIS_MAX; i++) {
    float val = 0.0f;

    if (m->axis_bind[i] >= 0 && m->axis_bind[i] < j->raw_axis_count) {
      val = j->raw_axes[m->axis_bind[i]];
      if (m->axis_inverted[i]) {
        val = -val;
      }
    }

    if (m->axis_button_pos[i] >= 0 || m->axis_button_neg[i] >= 0) {
      bool pos =
          m->axis_button_pos[i] >= 0 &&
          m->axis_button_pos[i] < j->raw_button_count &&
          j->raw_buttons[m->axis_button_pos[i]];
      bool neg =
          m->axis_button_neg[i] >= 0 &&
          m->axis_button_neg[i] < j->raw_button_count &&
          j->raw_buttons[m->axis_button_neg[i]];
      if (pos) val = 1.0f;
      if (neg) val = -1.0f;
    }

    j->axes[i] = gamepad_apply_deadzone(val, deadzone);
  }
}

// ============================================================================
// Platform: Windows (XInput)
// ============================================================================

#if defined(IS_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <xinput.h>

void gamepad_init(GamepadState *state) {
  memset(state, 0, sizeof(GamepadState));
  state->deadzone = 0.15f;
  state->mappings = {};
}

void gamepad_update(GamepadState *state) {
  for (DWORD i = 0; i < MAX_JOYSTICKS; i++) {
    Joystick *j = &state->joysticks[i];
    XINPUT_STATE xs;
    memset(&xs, 0, sizeof(xs));

    if (XInputGetState(i, &xs) == ERROR_SUCCESS) {
      j->connected = true;
      j->is_gamepad = true;
      j->platform_handle = (i64)i;
      snprintf(j->name, sizeof(j->name), "XInput Controller %d", (int)i);
      snprintf(j->guid, sizeof(j->guid), "xinput%d", (int)i);

      XINPUT_GAMEPAD *gp = &xs.Gamepad;
      float dz = state->deadzone;

      // Axes.
      j->axes[GAMEPAD_AXIS_LEFTX] =
          gamepad_apply_deadzone(gp->sThumbLX / 32767.0f, dz);
      j->axes[GAMEPAD_AXIS_LEFTY] =
          gamepad_apply_deadzone(-gp->sThumbLY / 32767.0f, dz);
      j->axes[GAMEPAD_AXIS_RIGHTX] =
          gamepad_apply_deadzone(gp->sThumbRX / 32767.0f, dz);
      j->axes[GAMEPAD_AXIS_RIGHTY] =
          gamepad_apply_deadzone(-gp->sThumbRY / 32767.0f, dz);
      j->axes[GAMEPAD_AXIS_LEFT_TRIGGER] = gp->bLeftTrigger / 255.0f;
      j->axes[GAMEPAD_AXIS_RIGHT_TRIGGER] = gp->bRightTrigger / 255.0f;

      // Buttons.
      j->buttons[GAMEPAD_BUTTON_A] = !!(gp->wButtons & XINPUT_GAMEPAD_A);
      j->buttons[GAMEPAD_BUTTON_B] = !!(gp->wButtons & XINPUT_GAMEPAD_B);
      j->buttons[GAMEPAD_BUTTON_X] = !!(gp->wButtons & XINPUT_GAMEPAD_X);
      j->buttons[GAMEPAD_BUTTON_Y] = !!(gp->wButtons & XINPUT_GAMEPAD_Y);
      j->buttons[GAMEPAD_BUTTON_BACK] = !!(gp->wButtons & XINPUT_GAMEPAD_BACK);
      j->buttons[GAMEPAD_BUTTON_START] =
          !!(gp->wButtons & XINPUT_GAMEPAD_START);
      j->buttons[GAMEPAD_BUTTON_LEFTSTICK] =
          !!(gp->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
      j->buttons[GAMEPAD_BUTTON_RIGHTSTICK] =
          !!(gp->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
      j->buttons[GAMEPAD_BUTTON_LEFTSHOULDER] =
          !!(gp->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
      j->buttons[GAMEPAD_BUTTON_RIGHTSHOULDER] =
          !!(gp->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
      j->buttons[GAMEPAD_BUTTON_DPUP] =
          !!(gp->wButtons & XINPUT_GAMEPAD_DPAD_UP);
      j->buttons[GAMEPAD_BUTTON_DPDOWN] =
          !!(gp->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
      j->buttons[GAMEPAD_BUTTON_DPLEFT] =
          !!(gp->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
      j->buttons[GAMEPAD_BUTTON_DPRIGHT] =
          !!(gp->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

      // Also populate raw state for consistency.
      j->raw_button_count = GAMEPAD_BUTTON_MAX;
      for (i32 b = 0; b < GAMEPAD_BUTTON_MAX; b++) {
        j->raw_buttons[b] = j->buttons[b];
      }
      j->raw_axis_count = GAMEPAD_AXIS_MAX;
      for (i32 a = 0; a < GAMEPAD_AXIS_MAX; a++) {
        j->raw_axes[a] = j->axes[a];
      }

    } else {
      if (j->connected) {
        memset(j, 0, sizeof(Joystick));
      }
      j->connected = false;
    }
  }
}

void gamepad_set_vibration(GamepadState *state, i32 index, float left,
                           float right, float duration) {
  (void)duration; // XInput doesn't natively support timed vibration.
  if (index < 0 || index >= MAX_JOYSTICKS) {
    return;
  }
  if (!state->joysticks[index].connected) {
    return;
  }
  XINPUT_VIBRATION vib;
  vib.wLeftMotorSpeed = (WORD)(left * 65535.0f);
  vib.wRightMotorSpeed = (WORD)(right * 65535.0f);
  XInputSetState((DWORD)index, &vib);
}

void gamepad_shutdown(GamepadState *state) {
  for (i32 i = 0; i < MAX_JOYSTICKS; i++) {
    gamepad_set_vibration(state, i, 0, 0, 0);
  }
  state->mappings.trash();
}

// ============================================================================
// Platform: Linux (evdev via /dev/input/js*)
// ============================================================================

#elif defined(IS_LINUX)

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

void gamepad_init(GamepadState *state) {
  memset(state, 0, sizeof(GamepadState));
  state->deadzone = 0.15f;
  state->mappings = {};

  for (i32 i = 0; i < MAX_JOYSTICKS; i++) {
    state->joysticks[i].platform_handle = -1;
  }
}

static void linux_open_joystick(GamepadState *state, i32 slot) {
  Joystick *j = &state->joysticks[slot];
  char path[64];
  snprintf(path, sizeof(path), "/dev/input/js%d", slot);

  int fd = open(path, O_RDONLY | O_NONBLOCK);
  if (fd < 0) {
    j->connected = false;
    j->platform_handle = -1;
    return;
  }

  j->connected = true;
  j->platform_handle = fd;

  // Get name.
  char name_buf[128] = {};
  if (ioctl(fd, JSIOCGNAME(sizeof(name_buf)), name_buf) < 0) {
    snprintf(name_buf, sizeof(name_buf), "Joystick %d", slot);
  }
  snprintf(j->name, sizeof(j->name), "%s", name_buf);

  // Number of axes and buttons.
  u8 axes_count = 0;
  u8 button_count = 0;
  ioctl(fd, JSIOCGAXES, &axes_count);
  ioctl(fd, JSIOCGBUTTONS, &button_count);
  j->raw_axis_count = axes_count < MAX_RAW_AXES ? axes_count : MAX_RAW_AXES;
  j->raw_button_count =
      button_count < MAX_RAW_BUTTONS ? button_count : MAX_RAW_BUTTONS;

  // Build a GUID from device info (simplified).
  // Real SDL2 uses bus/vendor/product/version from JSIOCGID.
  snprintf(j->guid, sizeof(j->guid), "linux%04x%04x%02x%02x", 0, 0,
           axes_count, button_count);

  // Try to find mapping.
  j->mapping_index = gamepad_find_mapping(state, j->guid);
  j->is_gamepad = j->mapping_index >= 0;
}

void gamepad_update(GamepadState *state) {
  for (i32 i = 0; i < MAX_JOYSTICKS; i++) {
    Joystick *j = &state->joysticks[i];

    if (j->platform_handle < 0) {
      linux_open_joystick(state, i);
      if (j->platform_handle < 0) {
        continue;
      }
    }

    int fd = (int)j->platform_handle;
    struct js_event ev;

    while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
      u8 type = ev.type & ~JS_EVENT_INIT;
      if (type == JS_EVENT_BUTTON) {
        if (ev.number < MAX_RAW_BUTTONS) {
          j->raw_buttons[ev.number] = ev.value != 0;
        }
      } else if (type == JS_EVENT_AXIS) {
        if (ev.number < MAX_RAW_AXES) {
          j->raw_axes[ev.number] = ev.value / 32767.0f;
        }
      }
    }

    if (errno == ENODEV) {
      close(fd);
      memset(j, 0, sizeof(Joystick));
      j->platform_handle = -1;
      j->connected = false;
      continue;
    }

    if (j->mapping_index >= 0) {
      apply_mapping(j, &state->mappings[j->mapping_index], state->deadzone);
    } else {
      // Default: pass through raw with deadzone.
      for (i32 a = 0; a < GAMEPAD_AXIS_MAX && a < j->raw_axis_count; a++) {
        j->axes[a] = gamepad_apply_deadzone(j->raw_axes[a], state->deadzone);
      }
      // Invert Y axes for screen coords.
      if (j->raw_axis_count > 1) {
        j->axes[GAMEPAD_AXIS_LEFTY] = -j->axes[GAMEPAD_AXIS_LEFTY];
      }
      if (j->raw_axis_count > 3) {
        j->axes[GAMEPAD_AXIS_RIGHTY] = -j->axes[GAMEPAD_AXIS_RIGHTY];
      }
      for (i32 b = 0; b < GAMEPAD_BUTTON_MAX && b < j->raw_button_count;
           b++) {
        j->buttons[b] = j->raw_buttons[b];
      }
    }
  }
}

void gamepad_set_vibration(GamepadState *, i32, float, float, float) {
  // Force feedback on Linux requires ff-memless, not implemented.
}

void gamepad_shutdown(GamepadState *state) {
  for (i32 i = 0; i < MAX_JOYSTICKS; i++) {
    Joystick *j = &state->joysticks[i];
    if (j->platform_handle >= 0) {
      close((int)j->platform_handle);
      j->platform_handle = -1;
    }
  }
  state->mappings.trash();
}

// ============================================================================
// Platform: Emscripten (HTML5 Gamepad API)
// ============================================================================

#elif defined(IS_HTML5)

#include <emscripten/html5.h>

void gamepad_init(GamepadState *state) {
  memset(state, 0, sizeof(GamepadState));
  state->deadzone = 0.15f;
  state->mappings = {};
}

void gamepad_update(GamepadState *state) {
  emscripten_sample_gamepad_data();
  int num = emscripten_get_num_gamepads();

  for (i32 i = 0; i < MAX_JOYSTICKS; i++) {
    Joystick *j = &state->joysticks[i];

    EmscriptenGamepadEvent ge;
    if (i < num &&
        emscripten_get_gamepad_status(i, &ge) == EMSCRIPTEN_RESULT_SUCCESS &&
        ge.connected) {
      j->connected = true;
      j->platform_handle = i;
      snprintf(j->name, sizeof(j->name), "%s", ge.id);
      snprintf(j->guid, sizeof(j->guid), "html5_%d", i);

      // The "standard" mapping in browsers maps to the W3C standard layout
      // which matches Xbox layout.
      j->is_gamepad = (ge.mapping[0] == 's'); // "standard"

      // Store raw.
      i32 naxes = ge.numAxes < MAX_RAW_AXES ? ge.numAxes : MAX_RAW_AXES;
      i32 nbtns =
          ge.numButtons < MAX_RAW_BUTTONS ? ge.numButtons : MAX_RAW_BUTTONS;
      j->raw_axis_count = naxes;
      j->raw_button_count = nbtns;
      for (i32 a = 0; a < naxes; a++) {
        j->raw_axes[a] = (float)ge.axis[a];
      }
      for (i32 b = 0; b < nbtns; b++) {
        j->raw_buttons[b] = ge.digitalButton[b] != 0;
      }

      if (j->is_gamepad) {
        // Standard mapping (W3C Gamepad API).
        float dz = state->deadzone;
        j->axes[GAMEPAD_AXIS_LEFTX] =
            naxes > 0 ? gamepad_apply_deadzone((float)ge.axis[0], dz) : 0;
        j->axes[GAMEPAD_AXIS_LEFTY] =
            naxes > 1 ? gamepad_apply_deadzone((float)ge.axis[1], dz) : 0;
        j->axes[GAMEPAD_AXIS_RIGHTX] =
            naxes > 2 ? gamepad_apply_deadzone((float)ge.axis[2], dz) : 0;
        j->axes[GAMEPAD_AXIS_RIGHTY] =
            naxes > 3 ? gamepad_apply_deadzone((float)ge.axis[3], dz) : 0;
        j->axes[GAMEPAD_AXIS_LEFT_TRIGGER] =
            nbtns > 6 ? (float)ge.analogButton[6] : 0;
        j->axes[GAMEPAD_AXIS_RIGHT_TRIGGER] =
            nbtns > 7 ? (float)ge.analogButton[7] : 0;

        if (nbtns > 0) j->buttons[GAMEPAD_BUTTON_A] = ge.digitalButton[0];
        if (nbtns > 1) j->buttons[GAMEPAD_BUTTON_B] = ge.digitalButton[1];
        if (nbtns > 2) j->buttons[GAMEPAD_BUTTON_X] = ge.digitalButton[2];
        if (nbtns > 3) j->buttons[GAMEPAD_BUTTON_Y] = ge.digitalButton[3];
        if (nbtns > 4)
          j->buttons[GAMEPAD_BUTTON_LEFTSHOULDER] = ge.digitalButton[4];
        if (nbtns > 5)
          j->buttons[GAMEPAD_BUTTON_RIGHTSHOULDER] = ge.digitalButton[5];
        // 6, 7 are triggers (analog).
        if (nbtns > 8)
          j->buttons[GAMEPAD_BUTTON_BACK] = ge.digitalButton[8];
        if (nbtns > 9)
          j->buttons[GAMEPAD_BUTTON_START] = ge.digitalButton[9];
        if (nbtns > 10)
          j->buttons[GAMEPAD_BUTTON_LEFTSTICK] = ge.digitalButton[10];
        if (nbtns > 11)
          j->buttons[GAMEPAD_BUTTON_RIGHTSTICK] = ge.digitalButton[11];
        if (nbtns > 12)
          j->buttons[GAMEPAD_BUTTON_DPUP] = ge.digitalButton[12];
        if (nbtns > 13)
          j->buttons[GAMEPAD_BUTTON_DPDOWN] = ge.digitalButton[13];
        if (nbtns > 14)
          j->buttons[GAMEPAD_BUTTON_DPLEFT] = ge.digitalButton[14];
        if (nbtns > 15)
          j->buttons[GAMEPAD_BUTTON_DPRIGHT] = ge.digitalButton[15];
        if (nbtns > 16)
          j->buttons[GAMEPAD_BUTTON_GUIDE] = ge.digitalButton[16];
      } else {
        // Non-standard: try mapping DB or pass through.
        j->mapping_index = gamepad_find_mapping(state, j->guid);
        if (j->mapping_index >= 0) {
          apply_mapping(j, &state->mappings[j->mapping_index], state->deadzone);
        }
      }
    } else {
      if (j->connected) {
        memset(j, 0, sizeof(Joystick));
      }
      j->connected = false;
    }
  }
}

void gamepad_set_vibration(GamepadState *, i32, float, float, float) {
  // Vibration not widely supported in browsers.
}

void gamepad_shutdown(GamepadState *state) { state->mappings.trash(); }

// ============================================================================
// Platform: Stub (unsupported)
// ============================================================================

#else

void gamepad_init(GamepadState *state) {
  memset(state, 0, sizeof(GamepadState));
  state->deadzone = 0.15f;
  state->mappings = {};
}

void gamepad_update(GamepadState *) {}
void gamepad_set_vibration(GamepadState *, i32, float, float, float) {}
void gamepad_shutdown(GamepadState *state) { state->mappings.trash(); }

#endif
