#pragma once

#include "array.h"
#include "prelude.h"

enum GamepadButton : i32 {
  GAMEPAD_BUTTON_A,
  GAMEPAD_BUTTON_B,
  GAMEPAD_BUTTON_X,
  GAMEPAD_BUTTON_Y,
  GAMEPAD_BUTTON_BACK,
  GAMEPAD_BUTTON_GUIDE,
  GAMEPAD_BUTTON_START,
  GAMEPAD_BUTTON_LEFTSTICK,
  GAMEPAD_BUTTON_RIGHTSTICK,
  GAMEPAD_BUTTON_LEFTSHOULDER,
  GAMEPAD_BUTTON_RIGHTSHOULDER,
  GAMEPAD_BUTTON_DPUP,
  GAMEPAD_BUTTON_DPDOWN,
  GAMEPAD_BUTTON_DPLEFT,
  GAMEPAD_BUTTON_DPRIGHT,
  GAMEPAD_BUTTON_MAX,
};

enum GamepadAxis : i32 {
  GAMEPAD_AXIS_LEFTX,
  GAMEPAD_AXIS_LEFTY,
  GAMEPAD_AXIS_RIGHTX,
  GAMEPAD_AXIS_RIGHTY,
  GAMEPAD_AXIS_LEFT_TRIGGER,
  GAMEPAD_AXIS_RIGHT_TRIGGER,
  GAMEPAD_AXIS_MAX,
};

#define MAX_JOYSTICKS 4
#define MAX_RAW_BUTTONS 32
#define MAX_RAW_AXES 8

// SDL2-compatible gamepad mapping entry.
// Format: "GUID,name,mapping_string"
// mapping_string is a comma-separated list of "target:source" pairs.
struct GamepadMapping {
  char guid[64];
  char name[128];

  // Maps standard button -> raw button index (-1 = unmapped).
  i32 button_bind[GAMEPAD_BUTTON_MAX];

  // For buttons mapped from axes (hat/trigger): axis index, direction.
  i32 button_axis[GAMEPAD_BUTTON_MAX];
  i32 button_axis_dir[GAMEPAD_BUTTON_MAX]; // +1 or -1

  // For buttons mapped from hats.
  i32 button_hat[GAMEPAD_BUTTON_MAX];
  i32 button_hat_mask[GAMEPAD_BUTTON_MAX];

  // Maps standard axis -> raw axis index (-1 = unmapped).
  i32 axis_bind[GAMEPAD_AXIS_MAX];
  bool axis_inverted[GAMEPAD_AXIS_MAX];

  // For axes mapped from buttons.
  i32 axis_button_pos[GAMEPAD_AXIS_MAX];
  i32 axis_button_neg[GAMEPAD_AXIS_MAX];
};

struct Joystick {
  bool connected;
  bool is_gamepad; // recognized via mapping DB

  char name[128];
  char guid[64];

  // Standard gamepad-mapped state.
  bool buttons[GAMEPAD_BUTTON_MAX];
  bool prev_buttons[GAMEPAD_BUTTON_MAX];
  float axes[GAMEPAD_AXIS_MAX];

  // Raw / unmapped state.
  bool raw_buttons[MAX_RAW_BUTTONS];
  float raw_axes[MAX_RAW_AXES];
  i32 raw_button_count;
  i32 raw_axis_count;

  // Index into mapping DB (-1 = none).
  i32 mapping_index;

  // Platform handle (XInput slot, fd, etc.).
  i64 platform_handle;
};

struct GamepadState {
  Joystick joysticks[MAX_JOYSTICKS];
  float deadzone; // default 0.15

  Array<GamepadMapping> mappings;
};

// Lifecycle.
void gamepad_init(GamepadState *state);
void gamepad_update(GamepadState *state);
void gamepad_end_frame(GamepadState *state);
void gamepad_shutdown(GamepadState *state);

// Rumble.
void gamepad_set_vibration(GamepadState *state, i32 index, float left,
                           float right, float duration);

// Queries.
i32 gamepad_count(GamepadState *state);
float gamepad_apply_deadzone(float value, float deadzone);

// SDL2-compatible mapping database.
// Load from a gamecontrollerdb.txt file (contents as a string).
i32 gamepad_add_mappings_from_string(GamepadState *state, String contents);

// Find mapping for a GUID. Returns index or -1.
i32 gamepad_find_mapping(GamepadState *state, const char *guid);
