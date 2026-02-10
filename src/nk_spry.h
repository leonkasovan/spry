#pragma once

#include "prelude.h"

struct sapp_event;
struct lua_State;

void nuklear_init();
void nuklear_trash();
void nuklear_sokol_event(const sapp_event *e);
void nuklear_begin();
void nuklear_end_and_present();

void open_nuklear_api(lua_State *L);
