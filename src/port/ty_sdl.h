/* The mini SDL of the port, its side towards the platform (ty_sdl.c) */
#ifndef TY_SDL_H
#define TY_SDL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ty_sdl_set_buttons(uint32_t buttons);
void ty_sdl_swap_ab(bool swap);
/* the game reads the player's keys (a level is being played) */
void ty_sdl_action(void);
bool ty_sdl_in_action(void);
/* mono output of the game's audio callback */
void ty_sdl_audio_render(int16_t *out, int frames);

#ifdef __cplusplus
}
#endif

#endif
