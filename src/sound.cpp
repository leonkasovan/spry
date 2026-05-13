#include "sound.h"
#include "app.h"
#include "hash_map.h"
#include "profile.h"

static HashMap<Sound *> g_sound_cache = {};

static void on_sound_end(void *udata, ma_sound *ma) {
  Sound *sound = (Sound *)udata;
  if (sound->zombie) {
    sound->dead_end = true;
  }
}

Sound *sound_load(String filepath) {
  PROFILE_FUNC();

  u64 hash = fnv1a(filepath);
  Sound **existing = g_sound_cache.get(hash);
  if (existing) {
    (*existing)->refcount++;
    return *existing;
  }

  ma_result res = MA_SUCCESS;

  Sound *sound = (Sound *)mem_alloc(sizeof(Sound));
  sound->refcount = 1;
  sound->path_hash = hash;

  String cpath = to_cstr(filepath);
  defer(mem_free(cpath.data));

  res = ma_sound_init_from_file(&g_app->audio_engine, cpath.data, 0, nullptr,
                                nullptr, &sound->ma);
  if (res != MA_SUCCESS) {
    mem_free(sound);
    return nullptr;
  }

  res = ma_sound_set_end_callback(&sound->ma, on_sound_end, sound);
  if (res != MA_SUCCESS) {
    mem_free(sound);
    return nullptr;
  }

  sound->zombie = false;
  sound->dead_end = false;
  g_sound_cache[hash] = sound;
  return sound;
}

void sound_unref(Sound *sound) {
  sound->refcount--;
  if (sound->refcount > 0) {
    return;
  }

  g_sound_cache.unset(sound->path_hash);

  if (ma_sound_at_end(&sound->ma)) {
    sound->trash();
    mem_free(sound);
  } else {
    sound->zombie = true;
    g_app->garbage_sounds.push(sound);
  }
}

void sound_cache_trash() {
  for (auto [k, v] : g_sound_cache) {
    (void)k;
    (*v)->trash();
    mem_free(*v);
  }
  g_sound_cache.trash();
}

void Sound::trash() {
  ma_sound_uninit(&ma);
}
