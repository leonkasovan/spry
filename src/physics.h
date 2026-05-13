#pragma once

#include <box2d/box2d.h>
#include "prelude.h"
#include "luax.h"

struct PhysicsUserData {
  i32 begin_contact_ref;
  i32 end_contact_ref;

  i32 ref_count;
  i32 type;
  union {
    char *str;
    lua_Number num;
  };
};

struct PhysicsContactListener;
struct Physics {
  b2World *world;
  PhysicsContactListener *contact_listener;
  float meter;

  union {
    b2Body *body;
    b2Fixture *fixture;
    b2Joint *joint;
  };
};

Physics physics_world_make(lua_State *L, b2Vec2 gravity, float meter);
void physics_world_trash(lua_State *L, Physics *p);
void physics_world_begin_contact(lua_State *L, Physics *p, i32 arg);
void physics_world_end_contact(lua_State *L, Physics *p, i32 arg);
void physics_world_presolve(lua_State *L, Physics *p, i32 arg);
void physics_world_postsolve(lua_State *L, Physics *p, i32 arg);
Physics physics_weak_copy(Physics *p);

void physics_destroy_body(lua_State *L, Physics *physics);
PhysicsUserData *physics_userdata(lua_State *L);
void physics_push_userdata(lua_State *L, u64 ptr);
bool physics_world_raycast(Physics *p, float x1, float y1, float x2, float y2,
                           b2Fixture **out_fixture, b2Vec2 *out_point, b2Vec2 *out_normal);
void draw_fixtures_for_body(b2Body *body, float meter);
