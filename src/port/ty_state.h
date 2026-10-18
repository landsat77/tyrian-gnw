/* Savestates of the port (ty_state.c): hooks in the game's code */
#ifndef TY_STATE_H
#define TY_STATE_H

#include <setjmp.h>
#include <stdbool.h>

/* JE_saveGame() does not write tyrian.sav (config.c) */
extern bool tyGnwNoSaveFile;

/* the title screen is shown (no game to save) */
void ty_state_title(void);
/* the game menu is entered / left */
void ty_state_item_screen(bool entering);
/* a savestate is waiting to be loaded (no intro) */
bool ty_state_pending(void);
/* title screen: load the pending savestate, true if a game was loaded */
bool ty_state_apply_pending(void);
/* ot_main(): setjmp(*ty_state_restart_env()) before its main loop */
jmp_buf *ty_state_restart_env(void);
void ty_state_after_restart(void);
/* ty_loudness.c */
void fade_song_reset(void);

#endif
