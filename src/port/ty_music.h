/* Pre-rendered music pieces (ty_music.c) */
#ifndef TY_MUSIC_H
#define TY_MUSIC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ty_mus ty_mus_t;

/* A "GWMU" piece in memory (read in place), NULL if invalid. */
ty_mus_t *ty_mus_open(const unsigned char *data, int len);
void ty_mus_close(ty_mus_t *f);
/* the piece plays once (it does not loop), its length at `out_rate` */
bool ty_mus_ends(const ty_mus_t *f);
uint32_t ty_mus_length(const ty_mus_t *f, int out_rate);
int ty_mus_rewind(ty_mus_t *f);
/* Adds the next `frames` frames at `out_rate` to `out` (interleaved,
 * `channels` channels) when `audible`. The piece loops forever (from its
 * loop start): returns `frames`. */
int ty_mus_mix(ty_mus_t *f, short *out, int frames, int channels, int audible, int out_rate);

#ifdef __cplusplus
}
#endif

#endif
