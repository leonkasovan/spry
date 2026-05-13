#pragma once

#include "deps/miniaudio.h"
#include "prelude.h"

struct Sound {
  ma_sound ma;
  bool zombie;
  bool dead_end;
  i32 refcount;
  u64 path_hash;

  void trash();
};

Sound *sound_load(String filepath);
void sound_unref(Sound *sound);
void sound_cache_trash();