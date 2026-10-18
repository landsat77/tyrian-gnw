/*
 * Interface between OpenTyrian (src/ot + the mini SDL of src/port/sdl) and
 * the platform it runs on:
 *  - src/port/ty_main.c : the Game & Watch homebrew (retro-go)
 *  - harness/ty_host.c  : a headless Linux harness used to test the port
 */
#ifndef TY_PLATFORM_H
#define TY_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The game's screen (VGA mode 13h) */
#define TY_WIDTH  320
#define TY_HEIGHT 200

/* Buttons of the console, as seen by the core (ty_core_set_buttons()) */
enum {
    TY_BTN_LEFT   = 1 << 0,
    TY_BTN_RIGHT  = 1 << 1,
    TY_BTN_UP     = 1 << 2,
    TY_BTN_DOWN   = 1 << 3,
    TY_BTN_A      = 1 << 4,
    TY_BTN_B      = 1 << 5,
    TY_BTN_GAME   = 1 << 6,   /* START on Zelda units */
    TY_BTN_TIME   = 1 << 7,   /* SELECT on Zelda units */
};

/* ---- implemented by the platform ---- */

/* Monotonic time in milliseconds. */
uint32_t ty_plat_time_ms(void);

/* Called while the game waits (in_delay = 1, SDL_Delay) and when it reads
 * its events (in_delay = 0, SDL_PollEvent). The platform feeds its audio
 * output with ty_core_audio_render(), reads its buttons and gives them to
 * ty_core_set_buttons(). */
void ty_plat_service(int in_delay);

/* Show the game's screen: TY_WIDTH x TY_HEIGHT palette indexes, `pitch`
 * bytes per line, with this palette (256 colours 0x00RRGGBB; `pal_version`
 * changes with it). */
void ty_plat_present(const uint8_t *pixels, int pitch,
                     const uint32_t *palette, uint32_t pal_version);

/* Output sample rate (mono, signed 16 bit). */
int ty_plat_audio_rate(void);

/* Look up a file of the asset pack (lower case names: "tyrian1.lvl"...).
 * The returned memory is read-only and stays valid while the game runs. */
bool ty_plat_find_file(const char *name, const uint8_t **data, uint32_t *size);

/* Directory (without trailing '/') where the game may write its files
 * (configuration, saved games). */
const char *ty_plat_save_dir(void);

/* Memory outside the heap for the big buffers of the game ("screen0".."2":
 * the 320x200 screens, "weapons": the weapon table), NULL: use the heap.
 * The memory is zeroed. */
void *ty_plat_big_memory(const char *what, size_t size);

/* Unrecoverable error. Never returns. */
void ty_plat_fatal(const char *msg) __attribute__((noreturn));

/* The game asked to quit. Never returns. */
void ty_plat_quit(void) __attribute__((noreturn));

/* ---- implemented by the core (ty_core.c, ty_sdl.c) ---- */

typedef struct {
    bool music;
    bool sound;
} ty_core_options_t;

/* Run the game. Never returns. */
void ty_core_run(const ty_core_options_t *opts) __attribute__((noreturn));

/* Buttons currently held (TY_BTN_*). */
void ty_core_set_buttons(uint32_t buttons);

/* Savestates (ty_state.c) */
bool ty_core_can_save(void);
bool ty_core_save_state(const char *path);
bool ty_core_load_state(const char *path);
/* After a service of the platform (outside its menu): a load waiting
 * restarts the game's main loop (longjmp). */
void ty_core_poll_restart(void);

/* Options of the pause menu */
void ty_core_set_sound(bool music, bool sound);

/* Render `frames` mono samples (sound effects and music). */
void ty_core_audio_render(int16_t *out, int frames);

/* ---- implemented by ty_alloc.c ---- */
void   ty_heap_init(void *base, size_t size);
void  *ty_malloc(size_t size);
void  *ty_calloc(size_t n, size_t size);
void  *ty_realloc(void *ptr, size_t size);
void   ty_free(void *ptr);
size_t ty_heap_used(void);
size_t ty_heap_peak(void);
size_t ty_heap_size(void);
void   ty_heap_ignore_range(const void *lo, const void *hi);

#ifdef __cplusplus
}
#endif

#endif
