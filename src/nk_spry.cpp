#include "nk_spry.h"

#include <cmath>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_BOOL
#define NK_KEYSTATE_BASED_INPUT
#define NK_IMPLEMENTATION
#include "deps/nuklear.h"

#include "app.h"
#include "deps/microui_atlas.inl"
#include "deps/sokol_app.h"
#include "deps/sokol_gfx.h"
#include "deps/sokol_gl.h"
#include "luax.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

// ---------------------------------------------------------------------------
// Nuklear state
// ---------------------------------------------------------------------------

struct NuklearState {
  struct nk_context ctx;
  struct nk_user_font font;
  u32 atlas;
};

static NuklearState g_nk_state;

static struct nk_context *nk_ctx() { return &g_nk_state.ctx; }

// ---------------------------------------------------------------------------
// Font using microui built-in atlas
// ---------------------------------------------------------------------------

static float nk_font_text_width(nk_handle handle, float height,
                                const char *text, int len) {
  (void)handle;
  (void)height;
  float w = 0;
  for (int i = 0; i < len && text[i]; i++) {
    w += mu_atlas_lookup(MU_ATLAS_FONT + (unsigned char)text[i]).w;
  }
  return w;
}

// ---------------------------------------------------------------------------
// Init / Trash
// ---------------------------------------------------------------------------

void nuklear_init() {
  // Build white texture (reuse microui atlas)
  u32 *bitmap = (u32 *)mem_alloc(MU_ATLAS_WIDTH * MU_ATLAS_HEIGHT * 4);
  for (i32 i = 0; i < MU_ATLAS_WIDTH * MU_ATLAS_HEIGHT; i++) {
    bitmap[i] = 0x00FFFFFF | ((u32)mu_atlas_texture[i] << 24);
  }

  sg_image_desc desc = {};
  desc.width = MU_ATLAS_WIDTH;
  desc.height = MU_ATLAS_HEIGHT;
  desc.data.subimage[0][0].ptr = bitmap;
  desc.data.subimage[0][0].size = MU_ATLAS_WIDTH * MU_ATLAS_HEIGHT * 4;
  g_nk_state.atlas = sg_make_image(&desc).id;

  mem_free(bitmap);

  // Setup font
  g_nk_state.font.height = 18.0f;
  g_nk_state.font.width = nk_font_text_width;

  nk_init_default(&g_nk_state.ctx, &g_nk_state.font);
}

void nuklear_trash() { nk_free(&g_nk_state.ctx); }

// ---------------------------------------------------------------------------
// Input from sokol_app
// ---------------------------------------------------------------------------

void nuklear_sokol_event(const sapp_event *e) {
  struct nk_context *ctx = nk_ctx();

  switch (e->type) {
  case SAPP_EVENTTYPE_MOUSE_DOWN:
    nk_input_button(ctx,
                    e->mouse_button == SAPP_MOUSEBUTTON_LEFT
                        ? NK_BUTTON_LEFT
                        : (e->mouse_button == SAPP_MOUSEBUTTON_RIGHT
                               ? NK_BUTTON_RIGHT
                               : NK_BUTTON_MIDDLE),
                    (int)e->mouse_x, (int)e->mouse_y, nk_true);
    break;
  case SAPP_EVENTTYPE_MOUSE_UP:
    nk_input_button(ctx,
                    e->mouse_button == SAPP_MOUSEBUTTON_LEFT
                        ? NK_BUTTON_LEFT
                        : (e->mouse_button == SAPP_MOUSEBUTTON_RIGHT
                               ? NK_BUTTON_RIGHT
                               : NK_BUTTON_MIDDLE),
                    (int)e->mouse_x, (int)e->mouse_y, nk_false);
    break;
  case SAPP_EVENTTYPE_MOUSE_MOVE:
    nk_input_motion(ctx, (int)e->mouse_x, (int)e->mouse_y);
    break;
  case SAPP_EVENTTYPE_MOUSE_SCROLL:
    nk_input_scroll(ctx, nk_vec2(e->scroll_x, e->scroll_y));
    break;
  case SAPP_EVENTTYPE_KEY_DOWN:
  case SAPP_EVENTTYPE_KEY_UP: {
    nk_bool down = (e->type == SAPP_EVENTTYPE_KEY_DOWN) ? nk_true : nk_false;
    switch (e->key_code) {
    case SAPP_KEYCODE_LEFT_SHIFT:
    case SAPP_KEYCODE_RIGHT_SHIFT:
      nk_input_key(ctx, NK_KEY_SHIFT, down);
      break;
    case SAPP_KEYCODE_LEFT_CONTROL:
    case SAPP_KEYCODE_RIGHT_CONTROL:
      nk_input_key(ctx, NK_KEY_CTRL, down);
      break;
    case SAPP_KEYCODE_DELETE:
      nk_input_key(ctx, NK_KEY_DEL, down);
      break;
    case SAPP_KEYCODE_ENTER:
      nk_input_key(ctx, NK_KEY_ENTER, down);
      break;
    case SAPP_KEYCODE_TAB:
      nk_input_key(ctx, NK_KEY_TAB, down);
      break;
    case SAPP_KEYCODE_BACKSPACE:
      nk_input_key(ctx, NK_KEY_BACKSPACE, down);
      break;
    case SAPP_KEYCODE_UP:
      nk_input_key(ctx, NK_KEY_UP, down);
      break;
    case SAPP_KEYCODE_DOWN:
      nk_input_key(ctx, NK_KEY_DOWN, down);
      break;
    case SAPP_KEYCODE_LEFT:
      nk_input_key(ctx, NK_KEY_LEFT, down);
      break;
    case SAPP_KEYCODE_RIGHT:
      nk_input_key(ctx, NK_KEY_RIGHT, down);
      break;
    case SAPP_KEYCODE_HOME:
      nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
      nk_input_key(ctx, NK_KEY_SCROLL_START, down);
      break;
    case SAPP_KEYCODE_END:
      nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
      nk_input_key(ctx, NK_KEY_SCROLL_END, down);
      break;
    case SAPP_KEYCODE_PAGE_DOWN:
      nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
      break;
    case SAPP_KEYCODE_PAGE_UP:
      nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
      break;
    default: break;
    }

    if (down && (e->modifiers & SAPP_MODIFIER_CTRL)) {
      switch (e->key_code) {
      case SAPP_KEYCODE_C:
        nk_input_key(ctx, NK_KEY_COPY, nk_true);
        nk_input_key(ctx, NK_KEY_COPY, nk_false);
        break;
      case SAPP_KEYCODE_X:
        nk_input_key(ctx, NK_KEY_CUT, nk_true);
        nk_input_key(ctx, NK_KEY_CUT, nk_false);
        break;
      case SAPP_KEYCODE_V:
        nk_input_key(ctx, NK_KEY_PASTE, nk_true);
        nk_input_key(ctx, NK_KEY_PASTE, nk_false);
        break;
      case SAPP_KEYCODE_Z:
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, nk_true);
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, nk_false);
        break;
      case SAPP_KEYCODE_Y:
        nk_input_key(ctx, NK_KEY_TEXT_REDO, nk_true);
        nk_input_key(ctx, NK_KEY_TEXT_REDO, nk_false);
        break;
      case SAPP_KEYCODE_A:
        nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, nk_true);
        nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, nk_false);
        break;
      default: break;
      }
    }
    break;
  }
  case SAPP_EVENTTYPE_CHAR:
    if (e->char_code >= 32) {
      nk_input_unicode(ctx, (nk_rune)e->char_code);
    }
    break;
  default: break;
  }
}

// ---------------------------------------------------------------------------
// Rendering: command-based via sokol_gl
// ---------------------------------------------------------------------------

static void nk_push_quad(float x0, float y0, float x1, float y1, float u0,
                         float v0, float u1, float v1, struct nk_color c) {
  sgl_begin_quads();
  sgl_c4b(c.r, c.g, c.b, c.a);
  sgl_v2f_t2f(x0, y0, u0, v0);
  sgl_v2f_t2f(x1, y0, u1, v0);
  sgl_v2f_t2f(x1, y1, u1, v1);
  sgl_v2f_t2f(x0, y1, u0, v1);
  sgl_end();
}

// White pixel in the atlas for solid rects/lines
static void nk_push_solid_quad(float x0, float y0, float x1, float y1,
                               struct nk_color c) {
  mu_Rect src = mu_atlas_lookup(MU_ATLAS_WHITE);
  float au = ((float)src.x + 0.5f) / (float)MU_ATLAS_WIDTH;
  float av = ((float)src.y + 0.5f) / (float)MU_ATLAS_HEIGHT;
  nk_push_quad(x0, y0, x1, y1, au, av, au, av, c);
}

void nuklear_begin() {
  nk_input_end(nk_ctx());
}

void nuklear_end_and_present() {
  struct nk_context *ctx = nk_ctx();

  sgl_enable_texture();
  sgl_texture({g_nk_state.atlas}, {});

  const struct nk_command *cmd;
  nk_foreach(cmd, ctx) {
    switch (cmd->type) {
    case NK_COMMAND_NOP: break;
    case NK_COMMAND_SCISSOR: {
      const struct nk_command_scissor *s =
          (const struct nk_command_scissor *)cmd;
      sgl_scissor_rect(s->x, s->y, s->w, s->h, true);
      break;
    }
    case NK_COMMAND_LINE: {
      const struct nk_command_line *l = (const struct nk_command_line *)cmd;
      float x0 = (float)l->begin.x;
      float y0 = (float)l->begin.y;
      float x1 = (float)l->end.x;
      float y1 = (float)l->end.y;
      float t = (float)l->line_thickness * 0.5f;
      // Simple thick line as quad
      float dx = x1 - x0, dy = y1 - y0;
      float len = sqrtf(dx * dx + dy * dy);
      if (len > 0) {
        float nx = -dy / len * t, ny = dx / len * t;
        sgl_begin_quads();
        sgl_c4b(l->color.r, l->color.g, l->color.b, l->color.a);
        mu_Rect ws = mu_atlas_lookup(MU_ATLAS_WHITE);
        float au = ((float)ws.x + 0.5f) / (float)MU_ATLAS_WIDTH;
        float av = ((float)ws.y + 0.5f) / (float)MU_ATLAS_HEIGHT;
        sgl_v2f_t2f(x0 + nx, y0 + ny, au, av);
        sgl_v2f_t2f(x0 - nx, y0 - ny, au, av);
        sgl_v2f_t2f(x1 - nx, y1 - ny, au, av);
        sgl_v2f_t2f(x1 + nx, y1 + ny, au, av);
        sgl_end();
      }
      break;
    }
    case NK_COMMAND_RECT: {
      const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
      float x = (float)r->x, y = (float)r->y;
      float w = (float)r->w, h = (float)r->h;
      float t = (float)r->line_thickness;
      // Top
      nk_push_solid_quad(x, y, x + w, y + t, r->color);
      // Bottom
      nk_push_solid_quad(x, y + h - t, x + w, y + h, r->color);
      // Left
      nk_push_solid_quad(x, y + t, x + t, y + h - t, r->color);
      // Right
      nk_push_solid_quad(x + w - t, y + t, x + w, y + h - t, r->color);
      break;
    }
    case NK_COMMAND_RECT_FILLED: {
      const struct nk_command_rect_filled *r =
          (const struct nk_command_rect_filled *)cmd;
      nk_push_solid_quad((float)r->x, (float)r->y, (float)(r->x + r->w),
                         (float)(r->y + r->h), r->color);
      break;
    }
    case NK_COMMAND_RECT_MULTI_COLOR: {
      const struct nk_command_rect_multi_color *r =
          (const struct nk_command_rect_multi_color *)cmd;
      // Approximate with single color (left)
      nk_push_solid_quad((float)r->x, (float)r->y, (float)(r->x + r->w),
                         (float)(r->y + r->h), r->left);
      break;
    }
    case NK_COMMAND_CIRCLE: {
      const struct nk_command_circle *c =
          (const struct nk_command_circle *)cmd;
      // Approximate circle as filled rect outline
      float cx = (float)c->x + c->w * 0.5f;
      float cy = (float)c->y + c->h * 0.5f;
      float r = c->w * 0.5f;
      float t = (float)c->line_thickness;
      nk_push_solid_quad(cx - r, cy - t * 0.5f, cx + r, cy + t * 0.5f,
                         c->color);
      nk_push_solid_quad(cx - t * 0.5f, cy - r, cx + t * 0.5f, cy + r,
                         c->color);
      break;
    }
    case NK_COMMAND_CIRCLE_FILLED: {
      const struct nk_command_circle_filled *c =
          (const struct nk_command_circle_filled *)cmd;
      nk_push_solid_quad((float)c->x, (float)c->y, (float)(c->x + c->w),
                         (float)(c->y + c->h), c->color);
      break;
    }
    case NK_COMMAND_TRIANGLE: {
      const struct nk_command_triangle *t =
          (const struct nk_command_triangle *)cmd;
      // Approximate with bounding rect outline
      float minx = (float)NK_MIN(t->a.x, NK_MIN(t->b.x, t->c.x));
      float miny = (float)NK_MIN(t->a.y, NK_MIN(t->b.y, t->c.y));
      float maxx = (float)NK_MAX(t->a.x, NK_MAX(t->b.x, t->c.x));
      float maxy = (float)NK_MAX(t->a.y, NK_MAX(t->b.y, t->c.y));
      nk_push_solid_quad(minx, miny, maxx, maxy, t->color);
      break;
    }
    case NK_COMMAND_TRIANGLE_FILLED: {
      const struct nk_command_triangle_filled *t =
          (const struct nk_command_triangle_filled *)cmd;
      sgl_begin_triangles();
      mu_Rect ws = mu_atlas_lookup(MU_ATLAS_WHITE);
      float au = ((float)ws.x + 0.5f) / (float)MU_ATLAS_WIDTH;
      float av = ((float)ws.y + 0.5f) / (float)MU_ATLAS_HEIGHT;
      sgl_c4b(t->color.r, t->color.g, t->color.b, t->color.a);
      sgl_v2f_t2f((float)t->a.x, (float)t->a.y, au, av);
      sgl_v2f_t2f((float)t->b.x, (float)t->b.y, au, av);
      sgl_v2f_t2f((float)t->c.x, (float)t->c.y, au, av);
      sgl_end();
      break;
    }
    case NK_COMMAND_TEXT: {
      const struct nk_command_text *t = (const struct nk_command_text *)cmd;
      float dx = (float)t->x;
      float dy = (float)t->y;
      for (int i = 0; i < t->length && t->string[i]; i++) {
        mu_Rect src =
            mu_atlas_lookup(MU_ATLAS_FONT + (unsigned char)t->string[i]);
        float su0 = (float)src.x / (float)MU_ATLAS_WIDTH;
        float sv0 = (float)src.y / (float)MU_ATLAS_HEIGHT;
        float su1 = (float)(src.x + src.w) / (float)MU_ATLAS_WIDTH;
        float sv1 = (float)(src.y + src.h) / (float)MU_ATLAS_HEIGHT;
        nk_push_quad(dx, dy, dx + (float)src.w, dy + (float)src.h, su0, sv0,
                     su1, sv1, t->foreground);
        dx += (float)src.w;
      }
      break;
    }
    case NK_COMMAND_IMAGE: {
      // Not supported with this simple atlas renderer
      break;
    }
    default: break;
    }
  }

  nk_clear(ctx);
  nk_input_begin(ctx);
}

// ---------------------------------------------------------------------------
// Lua API
// ---------------------------------------------------------------------------

#define NK_CTX() nk_ctx()

// -- Window --

static int nk_lua_begin(lua_State *L) {
  const char *title = luaL_checkstring(L, 1);
  float x = (float)luaL_checknumber(L, 2);
  float y = (float)luaL_checknumber(L, 3);
  float w = (float)luaL_checknumber(L, 4);
  float h = (float)luaL_checknumber(L, 5);
  nk_flags flags = (nk_flags)luaL_optinteger(L, 6, 0);
  nk_bool res = nk_begin(NK_CTX(), title, nk_rect(x, y, w, h), flags);
  lua_pushboolean(L, res);
  return 1;
}

static int nk_lua_end(lua_State *L) {
  (void)L;
  nk_end(NK_CTX());
  return 0;
}

// -- Window queries --

static int nk_lua_window_get_bounds(lua_State *L) {
  struct nk_rect r = nk_window_get_bounds(NK_CTX());
  lua_pushnumber(L, r.x);
  lua_pushnumber(L, r.y);
  lua_pushnumber(L, r.w);
  lua_pushnumber(L, r.h);
  return 4;
}

static int nk_lua_window_get_size(lua_State *L) {
  struct nk_vec2 s = nk_window_get_size(NK_CTX());
  lua_pushnumber(L, s.x);
  lua_pushnumber(L, s.y);
  return 2;
}

static int nk_lua_window_get_position(lua_State *L) {
  struct nk_vec2 p = nk_window_get_position(NK_CTX());
  lua_pushnumber(L, p.x);
  lua_pushnumber(L, p.y);
  return 2;
}

static int nk_lua_window_get_content_region(lua_State *L) {
  struct nk_rect r = nk_window_get_content_region(NK_CTX());
  lua_pushnumber(L, r.x);
  lua_pushnumber(L, r.y);
  lua_pushnumber(L, r.w);
  lua_pushnumber(L, r.h);
  return 4;
}

static int nk_lua_window_has_focus(lua_State *L) {
  lua_pushboolean(L, nk_window_has_focus(NK_CTX()));
  return 1;
}

static int nk_lua_window_is_hovered(lua_State *L) {
  lua_pushboolean(L, nk_window_is_hovered(NK_CTX()));
  return 1;
}

static int nk_lua_window_is_any_hovered(lua_State *L) {
  lua_pushboolean(L, nk_window_is_any_hovered(NK_CTX()));
  return 1;
}

static int nk_lua_item_is_any_active(lua_State *L) {
  lua_pushboolean(L, nk_item_is_any_active(NK_CTX()));
  return 1;
}

static int nk_lua_window_set_bounds(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  struct nk_rect r;
  r.x = (float)luaL_checknumber(L, 2);
  r.y = (float)luaL_checknumber(L, 3);
  r.w = (float)luaL_checknumber(L, 4);
  r.h = (float)luaL_checknumber(L, 5);
  nk_window_set_bounds(NK_CTX(), name, r);
  return 0;
}

static int nk_lua_window_set_position(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  struct nk_vec2 p;
  p.x = (float)luaL_checknumber(L, 2);
  p.y = (float)luaL_checknumber(L, 3);
  nk_window_set_position(NK_CTX(), name, p);
  return 0;
}

static int nk_lua_window_set_size(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  struct nk_vec2 s;
  s.x = (float)luaL_checknumber(L, 2);
  s.y = (float)luaL_checknumber(L, 3);
  nk_window_set_size(NK_CTX(), name, s);
  return 0;
}

static int nk_lua_window_set_focus(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  nk_window_set_focus(NK_CTX(), name);
  return 0;
}

static int nk_lua_window_close(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  nk_window_close(NK_CTX(), name);
  return 0;
}

static int nk_lua_window_collapse(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  enum nk_collapse_states state =
      (enum nk_collapse_states)luaL_checkinteger(L, 2);
  nk_window_collapse(NK_CTX(), name, state);
  return 0;
}

static int nk_lua_window_show(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  enum nk_show_states state = (enum nk_show_states)luaL_checkinteger(L, 2);
  nk_window_show(NK_CTX(), name, state);
  return 0;
}

static int nk_lua_window_is_collapsed(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_pushboolean(L, nk_window_is_collapsed(NK_CTX(), name));
  return 1;
}

static int nk_lua_window_is_closed(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_pushboolean(L, nk_window_is_closed(NK_CTX(), name));
  return 1;
}

static int nk_lua_window_is_hidden(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_pushboolean(L, nk_window_is_hidden(NK_CTX(), name));
  return 1;
}

static int nk_lua_window_is_active(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_pushboolean(L, nk_window_is_active(NK_CTX(), name));
  return 1;
}

// -- Layout --

static int nk_lua_layout_row_dynamic(lua_State *L) {
  float height = (float)luaL_checknumber(L, 1);
  int cols = (int)luaL_checkinteger(L, 2);
  nk_layout_row_dynamic(NK_CTX(), height, cols);
  return 0;
}

static int nk_lua_layout_row_static(lua_State *L) {
  float height = (float)luaL_checknumber(L, 1);
  int item_width = (int)luaL_checkinteger(L, 2);
  int cols = (int)luaL_checkinteger(L, 3);
  nk_layout_row_static(NK_CTX(), height, item_width, cols);
  return 0;
}

static int nk_lua_layout_row_begin(lua_State *L) {
  enum nk_layout_format fmt = (enum nk_layout_format)luaL_checkinteger(L, 1);
  float height = (float)luaL_checknumber(L, 2);
  int cols = (int)luaL_checkinteger(L, 3);
  nk_layout_row_begin(NK_CTX(), fmt, height, cols);
  return 0;
}

static int nk_lua_layout_row_push(lua_State *L) {
  float value = (float)luaL_checknumber(L, 1);
  nk_layout_row_push(NK_CTX(), value);
  return 0;
}

static int nk_lua_layout_row_end(lua_State *L) {
  (void)L;
  nk_layout_row_end(NK_CTX());
  return 0;
}

static int nk_lua_layout_space_begin(lua_State *L) {
  enum nk_layout_format fmt = (enum nk_layout_format)luaL_checkinteger(L, 1);
  float height = (float)luaL_checknumber(L, 2);
  int count = (int)luaL_checkinteger(L, 3);
  nk_layout_space_begin(NK_CTX(), fmt, height, count);
  return 0;
}

static int nk_lua_layout_space_push(lua_State *L) {
  struct nk_rect r;
  r.x = (float)luaL_checknumber(L, 1);
  r.y = (float)luaL_checknumber(L, 2);
  r.w = (float)luaL_checknumber(L, 3);
  r.h = (float)luaL_checknumber(L, 4);
  nk_layout_space_push(NK_CTX(), r);
  return 0;
}

static int nk_lua_layout_space_end(lua_State *L) {
  (void)L;
  nk_layout_space_end(NK_CTX());
  return 0;
}

static int nk_lua_spacer(lua_State *L) {
  (void)L;
  nk_spacer(NK_CTX());
  return 0;
}

// -- Group --

static int nk_lua_group_begin(lua_State *L) {
  const char *title = luaL_checkstring(L, 1);
  nk_flags flags = (nk_flags)luaL_optinteger(L, 2, 0);
  lua_pushboolean(L, nk_group_begin(NK_CTX(), title, flags));
  return 1;
}

static int nk_lua_group_end(lua_State *L) {
  (void)L;
  nk_group_end(NK_CTX());
  return 0;
}

// -- Tree --

static int nk_lua_tree_push(lua_State *L) {
  enum nk_tree_type type = (enum nk_tree_type)luaL_checkinteger(L, 1);
  const char *title = luaL_checkstring(L, 2);
  enum nk_collapse_states state =
      (enum nk_collapse_states)luaL_optinteger(L, 3, NK_MINIMIZED);
  nk_bool res =
      nk_tree_push_hashed(NK_CTX(), type, title, state, title,
                          (int)strlen(title), (int)lua_tointeger(L, 4));
  lua_pushboolean(L, res);
  return 1;
}

static int nk_lua_tree_pop(lua_State *L) {
  (void)L;
  nk_tree_pop(NK_CTX());
  return 0;
}

// -- Widgets: text / label --

static int nk_lua_label(lua_State *L) {
  const char *text = luaL_checkstring(L, 1);
  nk_flags align = (nk_flags)luaL_optinteger(L, 2, NK_TEXT_LEFT);
  nk_label(NK_CTX(), text, align);
  return 0;
}

static int nk_lua_label_colored(lua_State *L) {
  const char *text = luaL_checkstring(L, 1);
  nk_flags align = (nk_flags)luaL_checkinteger(L, 2);
  struct nk_color c;
  c.r = (nk_byte)luaL_checkinteger(L, 3);
  c.g = (nk_byte)luaL_checkinteger(L, 4);
  c.b = (nk_byte)luaL_checkinteger(L, 5);
  c.a = (nk_byte)luaL_optinteger(L, 6, 255);
  nk_label_colored(NK_CTX(), text, align, c);
  return 0;
}

static int nk_lua_label_wrap(lua_State *L) {
  const char *text = luaL_checkstring(L, 1);
  nk_label_wrap(NK_CTX(), text);
  return 0;
}

// -- Button --

static int nk_lua_button_label(lua_State *L) {
  const char *title = luaL_checkstring(L, 1);
  lua_pushboolean(L, nk_button_label(NK_CTX(), title));
  return 1;
}

static int nk_lua_button_color(lua_State *L) {
  struct nk_color c;
  c.r = (nk_byte)luaL_checkinteger(L, 1);
  c.g = (nk_byte)luaL_checkinteger(L, 2);
  c.b = (nk_byte)luaL_checkinteger(L, 3);
  c.a = (nk_byte)luaL_optinteger(L, 4, 255);
  lua_pushboolean(L, nk_button_color(NK_CTX(), c));
  return 1;
}

static int nk_lua_button_symbol(lua_State *L) {
  enum nk_symbol_type sym = (enum nk_symbol_type)luaL_checkinteger(L, 1);
  lua_pushboolean(L, nk_button_symbol(NK_CTX(), sym));
  return 1;
}

static int nk_lua_button_symbol_label(lua_State *L) {
  enum nk_symbol_type sym = (enum nk_symbol_type)luaL_checkinteger(L, 1);
  const char *label = luaL_checkstring(L, 2);
  nk_flags align = (nk_flags)luaL_optinteger(L, 3, NK_TEXT_LEFT);
  lua_pushboolean(L, nk_button_symbol_label(NK_CTX(), sym, label, align));
  return 1;
}

// -- Checkbox --

static int nk_lua_checkbox_label(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  nk_bool active = lua_toboolean(L, 2) ? nk_true : nk_false;
  nk_checkbox_label(NK_CTX(), label, &active);
  lua_pushboolean(L, active);
  return 1;
}

// -- Option / Radio --

static int nk_lua_option_label(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  nk_bool active = lua_toboolean(L, 2) ? nk_true : nk_false;
  nk_bool result = nk_option_label(NK_CTX(), label, active);
  lua_pushboolean(L, result);
  return 1;
}

// -- Selectable --

static int nk_lua_selectable_label(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  nk_flags align = (nk_flags)luaL_checkinteger(L, 2);
  nk_bool value = lua_toboolean(L, 3) ? nk_true : nk_false;
  nk_selectable_label(NK_CTX(), label, align, &value);
  lua_pushboolean(L, value);
  return 1;
}

// -- Slider --

static int nk_lua_slider_float(lua_State *L) {
  float min_val = (float)luaL_checknumber(L, 1);
  float val = (float)luaL_checknumber(L, 2);
  float max_val = (float)luaL_checknumber(L, 3);
  float step = (float)luaL_checknumber(L, 4);
  nk_slider_float(NK_CTX(), min_val, &val, max_val, step);
  lua_pushnumber(L, val);
  return 1;
}

static int nk_lua_slider_int(lua_State *L) {
  int min_val = (int)luaL_checkinteger(L, 1);
  int val = (int)luaL_checkinteger(L, 2);
  int max_val = (int)luaL_checkinteger(L, 3);
  int step = (int)luaL_checkinteger(L, 4);
  nk_slider_int(NK_CTX(), min_val, &val, max_val, step);
  lua_pushinteger(L, val);
  return 1;
}

// -- Progress --

static int nk_lua_progress(lua_State *L) {
  nk_size cur = (nk_size)luaL_checkinteger(L, 1);
  nk_size max_val = (nk_size)luaL_checkinteger(L, 2);
  nk_bool modifiable = lua_toboolean(L, 3) ? nk_true : nk_false;
  nk_progress(NK_CTX(), &cur, max_val, modifiable);
  lua_pushinteger(L, (lua_Integer)cur);
  return 1;
}

// -- Property --

static int nk_lua_property_float(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  float min_val = (float)luaL_checknumber(L, 2);
  float val = (float)luaL_checknumber(L, 3);
  float max_val = (float)luaL_checknumber(L, 4);
  float step = (float)luaL_checknumber(L, 5);
  float inc = (float)luaL_checknumber(L, 6);
  nk_property_float(NK_CTX(), name, min_val, &val, max_val, step, inc);
  lua_pushnumber(L, val);
  return 1;
}

static int nk_lua_property_int(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  int min_val = (int)luaL_checkinteger(L, 2);
  int val = (int)luaL_checkinteger(L, 3);
  int max_val = (int)luaL_checkinteger(L, 4);
  int step = (int)luaL_checkinteger(L, 5);
  float inc = (float)luaL_checknumber(L, 6);
  nk_property_int(NK_CTX(), name, min_val, &val, max_val, step, inc);
  lua_pushinteger(L, val);
  return 1;
}

// -- Edit / Textbox --

static int nk_lua_edit_string(lua_State *L) {
  nk_flags flags = (nk_flags)luaL_checkinteger(L, 1);
  size_t slen;
  const char *src = luaL_checklstring(L, 2, &slen);
  int max_len = (int)luaL_optinteger(L, 3, 256);

  char *buf = (char *)mem_alloc(max_len + 1);
  int len = (int)(slen < (size_t)max_len ? slen : (size_t)max_len);
  memcpy(buf, src, len);
  buf[len] = '\0';

  nk_flags result =
      nk_edit_string_zero_terminated(NK_CTX(), flags, buf, max_len, nullptr);

  lua_pushinteger(L, result);
  lua_pushstring(L, buf);
  mem_free(buf);
  return 2;
}

// -- Color picker --

static int nk_lua_color_picker(lua_State *L) {
  struct nk_colorf c;
  c.r = (float)luaL_checknumber(L, 1);
  c.g = (float)luaL_checknumber(L, 2);
  c.b = (float)luaL_checknumber(L, 3);
  c.a = (float)luaL_optnumber(L, 4, 1.0);
  enum nk_color_format fmt = (enum nk_color_format)luaL_optinteger(L, 5, NK_RGBA);
  c = nk_color_picker(NK_CTX(), c, fmt);
  lua_pushnumber(L, c.r);
  lua_pushnumber(L, c.g);
  lua_pushnumber(L, c.b);
  lua_pushnumber(L, c.a);
  return 4;
}

// -- Combo --

static int nk_lua_combo(lua_State *L) {
  // combo(items_table, selected, item_height, width, height)
  luaL_checktype(L, 1, LUA_TTABLE);
  int count = (int)luaL_len(L, 1);
  int selected = (int)luaL_checkinteger(L, 2) - 1; // 1-indexed to 0-indexed
  int item_height = (int)luaL_checkinteger(L, 3);
  float w = (float)luaL_checknumber(L, 4);
  float h = (float)luaL_checknumber(L, 5);

  const char **items = (const char **)mem_alloc(count * sizeof(const char *));
  for (int i = 0; i < count; i++) {
    lua_rawgeti(L, 1, i + 1);
    items[i] = lua_tostring(L, -1);
    lua_pop(L, 1);
  }

  // Push items back on for lifetime
  for (int i = 0; i < count; i++) {
    lua_rawgeti(L, 1, i + 1);
  }

  int result = nk_combo(NK_CTX(), items, count, selected, item_height,
                        nk_vec2(w, h));

  lua_settop(L, 5); // clean up stack
  mem_free(items);

  lua_pushinteger(L, result + 1); // back to 1-indexed
  return 1;
}

// -- Popup --

static int nk_lua_popup_begin(lua_State *L) {
  enum nk_popup_type type = (enum nk_popup_type)luaL_checkinteger(L, 1);
  const char *title = luaL_checkstring(L, 2);
  nk_flags flags = (nk_flags)luaL_checkinteger(L, 3);
  struct nk_rect r;
  r.x = (float)luaL_checknumber(L, 4);
  r.y = (float)luaL_checknumber(L, 5);
  r.w = (float)luaL_checknumber(L, 6);
  r.h = (float)luaL_checknumber(L, 7);
  lua_pushboolean(L, nk_popup_begin(NK_CTX(), type, title, flags, r));
  return 1;
}

static int nk_lua_popup_close(lua_State *L) {
  (void)L;
  nk_popup_close(NK_CTX());
  return 0;
}

static int nk_lua_popup_end(lua_State *L) {
  (void)L;
  nk_popup_end(NK_CTX());
  return 0;
}

// -- Contextual --

static int nk_lua_contextual_begin(lua_State *L) {
  nk_flags flags = (nk_flags)luaL_checkinteger(L, 1);
  float w = (float)luaL_checknumber(L, 2);
  float h = (float)luaL_checknumber(L, 3);
  struct nk_rect trigger;
  trigger.x = (float)luaL_checknumber(L, 4);
  trigger.y = (float)luaL_checknumber(L, 5);
  trigger.w = (float)luaL_checknumber(L, 6);
  trigger.h = (float)luaL_checknumber(L, 7);
  lua_pushboolean(
      L, nk_contextual_begin(NK_CTX(), flags, nk_vec2(w, h), trigger));
  return 1;
}

static int nk_lua_contextual_item_label(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  nk_flags align = (nk_flags)luaL_optinteger(L, 2, NK_TEXT_LEFT);
  lua_pushboolean(L, nk_contextual_item_label(NK_CTX(), label, align));
  return 1;
}

static int nk_lua_contextual_close(lua_State *L) {
  (void)L;
  nk_contextual_close(NK_CTX());
  return 0;
}

static int nk_lua_contextual_end(lua_State *L) {
  (void)L;
  nk_contextual_end(NK_CTX());
  return 0;
}

// -- Tooltip --

static int nk_lua_tooltip(lua_State *L) {
  const char *text = luaL_checkstring(L, 1);
  nk_tooltip(NK_CTX(), text);
  return 0;
}

static int nk_lua_tooltip_begin(lua_State *L) {
  float width = (float)luaL_checknumber(L, 1);
  lua_pushboolean(L, nk_tooltip_begin(NK_CTX(), width));
  return 1;
}

static int nk_lua_tooltip_end(lua_State *L) {
  (void)L;
  nk_tooltip_end(NK_CTX());
  return 0;
}

// -- Menubar --

static int nk_lua_menubar_begin(lua_State *L) {
  (void)L;
  nk_menubar_begin(NK_CTX());
  return 0;
}

static int nk_lua_menubar_end(lua_State *L) {
  (void)L;
  nk_menubar_end(NK_CTX());
  return 0;
}

static int nk_lua_menu_begin_label(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  nk_flags align = (nk_flags)luaL_checkinteger(L, 2);
  float w = (float)luaL_checknumber(L, 3);
  float h = (float)luaL_checknumber(L, 4);
  lua_pushboolean(
      L, nk_menu_begin_label(NK_CTX(), label, align, nk_vec2(w, h)));
  return 1;
}

static int nk_lua_menu_item_label(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  nk_flags align = (nk_flags)luaL_optinteger(L, 2, NK_TEXT_LEFT);
  lua_pushboolean(L, nk_menu_item_label(NK_CTX(), label, align));
  return 1;
}

static int nk_lua_menu_close(lua_State *L) {
  (void)L;
  nk_menu_close(NK_CTX());
  return 0;
}

static int nk_lua_menu_end(lua_State *L) {
  (void)L;
  nk_end(NK_CTX());
  return 0;
}

// -- Chart --

static int nk_lua_chart_begin(lua_State *L) {
  enum nk_chart_type type = (enum nk_chart_type)luaL_checkinteger(L, 1);
  int num = (int)luaL_checkinteger(L, 2);
  float min_val = (float)luaL_checknumber(L, 3);
  float max_val = (float)luaL_checknumber(L, 4);
  lua_pushboolean(L, nk_chart_begin(NK_CTX(), type, num, min_val, max_val));
  return 1;
}

static int nk_lua_chart_push(lua_State *L) {
  float val = (float)luaL_checknumber(L, 1);
  nk_flags flags = nk_chart_push(NK_CTX(), val);
  lua_pushinteger(L, flags);
  return 1;
}

static int nk_lua_chart_end(lua_State *L) {
  (void)L;
  nk_chart_end(NK_CTX());
  return 0;
}

// -- Horizontal rule --

static int nk_lua_rule_horizontal(lua_State *L) {
  struct nk_color c;
  c.r = (nk_byte)luaL_checkinteger(L, 1);
  c.g = (nk_byte)luaL_checkinteger(L, 2);
  c.b = (nk_byte)luaL_checkinteger(L, 3);
  c.a = (nk_byte)luaL_optinteger(L, 4, 255);
  nk_bool rounding = lua_toboolean(L, 5) ? nk_true : nk_false;
  nk_rule_horizontal(NK_CTX(), c, rounding);
  return 0;
}

// ---------------------------------------------------------------------------
// Module registration
// ---------------------------------------------------------------------------

void open_nuklear_api(lua_State *L) {
  luaL_Reg reg[] = {
      // Window
      {"window_begin", nk_lua_begin},
      {"window_end", nk_lua_end},
      {"window_get_bounds", nk_lua_window_get_bounds},
      {"window_get_size", nk_lua_window_get_size},
      {"window_get_position", nk_lua_window_get_position},
      {"window_get_content_region", nk_lua_window_get_content_region},
      {"window_has_focus", nk_lua_window_has_focus},
      {"window_is_hovered", nk_lua_window_is_hovered},
      {"window_is_any_hovered", nk_lua_window_is_any_hovered},
      {"item_is_any_active", nk_lua_item_is_any_active},
      {"window_set_bounds", nk_lua_window_set_bounds},
      {"window_set_position", nk_lua_window_set_position},
      {"window_set_size", nk_lua_window_set_size},
      {"window_set_focus", nk_lua_window_set_focus},
      {"window_close", nk_lua_window_close},
      {"window_collapse", nk_lua_window_collapse},
      {"window_show", nk_lua_window_show},
      {"window_is_collapsed", nk_lua_window_is_collapsed},
      {"window_is_closed", nk_lua_window_is_closed},
      {"window_is_hidden", nk_lua_window_is_hidden},
      {"window_is_active", nk_lua_window_is_active},

      // Layout
      {"layout_row_dynamic", nk_lua_layout_row_dynamic},
      {"layout_row_static", nk_lua_layout_row_static},
      {"layout_row_begin", nk_lua_layout_row_begin},
      {"layout_row_push", nk_lua_layout_row_push},
      {"layout_row_end", nk_lua_layout_row_end},
      {"layout_space_begin", nk_lua_layout_space_begin},
      {"layout_space_push", nk_lua_layout_space_push},
      {"layout_space_end", nk_lua_layout_space_end},
      {"spacer", nk_lua_spacer},

      // Group
      {"group_begin", nk_lua_group_begin},
      {"group_end", nk_lua_group_end},

      // Tree
      {"tree_push", nk_lua_tree_push},
      {"tree_pop", nk_lua_tree_pop},

      // Text / Label
      {"label", nk_lua_label},
      {"label_colored", nk_lua_label_colored},
      {"label_wrap", nk_lua_label_wrap},

      // Button
      {"button_label", nk_lua_button_label},
      {"button_color", nk_lua_button_color},
      {"button_symbol", nk_lua_button_symbol},
      {"button_symbol_label", nk_lua_button_symbol_label},

      // Checkbox / Option
      {"checkbox_label", nk_lua_checkbox_label},
      {"option_label", nk_lua_option_label},

      // Selectable
      {"selectable_label", nk_lua_selectable_label},

      // Slider
      {"slider_float", nk_lua_slider_float},
      {"slider_int", nk_lua_slider_int},

      // Progress
      {"progress", nk_lua_progress},

      // Property
      {"property_float", nk_lua_property_float},
      {"property_int", nk_lua_property_int},

      // Edit
      {"edit_string", nk_lua_edit_string},

      // Color picker
      {"color_picker", nk_lua_color_picker},

      // Combo
      {"combo", nk_lua_combo},

      // Popup
      {"popup_begin", nk_lua_popup_begin},
      {"popup_close", nk_lua_popup_close},
      {"popup_end", nk_lua_popup_end},

      // Contextual
      {"contextual_begin", nk_lua_contextual_begin},
      {"contextual_item_label", nk_lua_contextual_item_label},
      {"contextual_close", nk_lua_contextual_close},
      {"contextual_end", nk_lua_contextual_end},

      // Tooltip
      {"tooltip", nk_lua_tooltip},
      {"tooltip_begin", nk_lua_tooltip_begin},
      {"tooltip_end", nk_lua_tooltip_end},

      // Menubar
      {"menubar_begin", nk_lua_menubar_begin},
      {"menubar_end", nk_lua_menubar_end},
      {"menu_begin_label", nk_lua_menu_begin_label},
      {"menu_item_label", nk_lua_menu_item_label},
      {"menu_close", nk_lua_menu_close},
      {"menu_end", nk_lua_menu_end},

      // Chart
      {"chart_begin", nk_lua_chart_begin},
      {"chart_push", nk_lua_chart_push},
      {"chart_end", nk_lua_chart_end},

      // Misc
      {"rule_horizontal", nk_lua_rule_horizontal},

      {nullptr, nullptr},
  };

  luaL_newlib(L, reg);

  // Window flags
  luax_set_int_field(L, "WINDOW_BORDER", NK_WINDOW_BORDER);
  luax_set_int_field(L, "WINDOW_MOVABLE", NK_WINDOW_MOVABLE);
  luax_set_int_field(L, "WINDOW_SCALABLE", NK_WINDOW_SCALABLE);
  luax_set_int_field(L, "WINDOW_CLOSABLE", NK_WINDOW_CLOSABLE);
  luax_set_int_field(L, "WINDOW_MINIMIZABLE", NK_WINDOW_MINIMIZABLE);
  luax_set_int_field(L, "WINDOW_NO_SCROLLBAR", NK_WINDOW_NO_SCROLLBAR);
  luax_set_int_field(L, "WINDOW_TITLE", NK_WINDOW_TITLE);
  luax_set_int_field(L, "WINDOW_SCROLL_AUTO_HIDE", NK_WINDOW_SCROLL_AUTO_HIDE);
  luax_set_int_field(L, "WINDOW_BACKGROUND", NK_WINDOW_BACKGROUND);
  luax_set_int_field(L, "WINDOW_SCALE_LEFT", NK_WINDOW_SCALE_LEFT);
  luax_set_int_field(L, "WINDOW_NO_INPUT", NK_WINDOW_NO_INPUT);

  // Text alignment
  luax_set_int_field(L, "TEXT_LEFT", NK_TEXT_LEFT);
  luax_set_int_field(L, "TEXT_CENTERED", NK_TEXT_CENTERED);
  luax_set_int_field(L, "TEXT_RIGHT", NK_TEXT_RIGHT);

  // Layout format
  luax_set_int_field(L, "DYNAMIC", NK_DYNAMIC);
  luax_set_int_field(L, "STATIC", NK_STATIC);

  // Tree type
  luax_set_int_field(L, "TREE_NODE", NK_TREE_NODE);
  luax_set_int_field(L, "TREE_TAB", NK_TREE_TAB);

  // Collapse states
  luax_set_int_field(L, "MINIMIZED", NK_MINIMIZED);
  luax_set_int_field(L, "MAXIMIZED", NK_MAXIMIZED);

  // Show states
  luax_set_int_field(L, "HIDDEN", NK_HIDDEN);
  luax_set_int_field(L, "SHOWN", NK_SHOWN);

  // Symbol types
  luax_set_int_field(L, "SYMBOL_NONE", NK_SYMBOL_NONE);
  luax_set_int_field(L, "SYMBOL_X", NK_SYMBOL_X);
  luax_set_int_field(L, "SYMBOL_CIRCLE_SOLID", NK_SYMBOL_CIRCLE_SOLID);
  luax_set_int_field(L, "SYMBOL_CIRCLE_OUTLINE", NK_SYMBOL_CIRCLE_OUTLINE);
  luax_set_int_field(L, "SYMBOL_RECT_SOLID", NK_SYMBOL_RECT_SOLID);
  luax_set_int_field(L, "SYMBOL_RECT_OUTLINE", NK_SYMBOL_RECT_OUTLINE);
  luax_set_int_field(L, "SYMBOL_TRIANGLE_UP", NK_SYMBOL_TRIANGLE_UP);
  luax_set_int_field(L, "SYMBOL_TRIANGLE_DOWN", NK_SYMBOL_TRIANGLE_DOWN);
  luax_set_int_field(L, "SYMBOL_TRIANGLE_LEFT", NK_SYMBOL_TRIANGLE_LEFT);
  luax_set_int_field(L, "SYMBOL_TRIANGLE_RIGHT", NK_SYMBOL_TRIANGLE_RIGHT);
  luax_set_int_field(L, "SYMBOL_PLUS", NK_SYMBOL_PLUS);
  luax_set_int_field(L, "SYMBOL_MINUS", NK_SYMBOL_MINUS);

  // Popup type
  luax_set_int_field(L, "POPUP_STATIC", NK_POPUP_STATIC);
  luax_set_int_field(L, "POPUP_DYNAMIC", NK_POPUP_DYNAMIC);

  // Chart type
  luax_set_int_field(L, "CHART_LINES", NK_CHART_LINES);
  luax_set_int_field(L, "CHART_COLUMN", NK_CHART_COLUMN);

  // Chart events
  luax_set_int_field(L, "CHART_HOVERING", NK_CHART_HOVERING);
  luax_set_int_field(L, "CHART_CLICKED", NK_CHART_CLICKED);

  // Color format
  luax_set_int_field(L, "RGB", NK_RGB);
  luax_set_int_field(L, "RGBA", NK_RGBA);

  // Edit flags
  luax_set_int_field(L, "EDIT_DEFAULT", NK_EDIT_DEFAULT);
  luax_set_int_field(L, "EDIT_READ_ONLY", NK_EDIT_READ_ONLY);
  luax_set_int_field(L, "EDIT_AUTO_SELECT", NK_EDIT_AUTO_SELECT);
  luax_set_int_field(L, "EDIT_SIG_ENTER", NK_EDIT_SIG_ENTER);
  luax_set_int_field(L, "EDIT_ALLOW_TAB", NK_EDIT_ALLOW_TAB);
  luax_set_int_field(L, "EDIT_NO_CURSOR", NK_EDIT_NO_CURSOR);
  luax_set_int_field(L, "EDIT_SELECTABLE", NK_EDIT_SELECTABLE);
  luax_set_int_field(L, "EDIT_CLIPBOARD", NK_EDIT_CLIPBOARD);
  luax_set_int_field(L, "EDIT_MULTILINE", NK_EDIT_MULTILINE);
  luax_set_int_field(L, "EDIT_SIMPLE", NK_EDIT_SIMPLE);
  luax_set_int_field(L, "EDIT_FIELD", NK_EDIT_FIELD);
  luax_set_int_field(L, "EDIT_BOX", NK_EDIT_BOX);
  luax_set_int_field(L, "EDIT_EDITOR", NK_EDIT_EDITOR);

  // Edit events
  luax_set_int_field(L, "EDIT_ACTIVE", NK_EDIT_ACTIVE);
  luax_set_int_field(L, "EDIT_INACTIVE", NK_EDIT_INACTIVE);
  luax_set_int_field(L, "EDIT_ACTIVATED", NK_EDIT_ACTIVATED);
  luax_set_int_field(L, "EDIT_DEACTIVATED", NK_EDIT_DEACTIVATED);
  luax_set_int_field(L, "EDIT_COMMITED", NK_EDIT_COMMITED);
}
