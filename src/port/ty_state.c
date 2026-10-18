/*
 * Savestates of Tyrian on the Game & Watch.
 *
 * Tyrian saves games between the levels only (the game menu): a savestate
 * is such a saved game (JE_SaveFileType, what the game's own Save writes),
 * taken when the game menu is left to play a level, or in the game menu
 * itself. Loading it goes back to the game menu before that level, with
 * the ship, the items and the money of then, like the game's Load Game.
 *
 * A load while the game runs sets a pending load and restarts the game's
 * main loop (ot_main(), setjmp before its title screen loop) at the next
 * platform service; the title screen then loads the pending game.
 */
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "episodes.h"
#include "keyboard.h"
#include "loudness.h"
#include "mainint.h"
#include "opentyr.h"
#include "player.h"
#include "varz.h"

#include "ty_platform.h"
#include "ty_state.h"

#define STATE_MAGIC 0x31535954u /* "TYS1" */

typedef struct {
	Uint32 magic;
	Uint32 size;
	Uint32 two_players;
	JE_SaveFileType save;
} ty_state_file_t;

bool tyGnwNoSaveFile;

static JE_SaveFileType snap;
static bool snap_valid, snap_two;
static bool in_item_screen;

static ty_state_file_t pending;
static bool pending_valid;
static bool restart_requested;

static jmp_buf restart_env;
static bool restart_ready;

static int slot_index(bool two)
{
	return two ? 11 : 0;
}

/* the game as its own Save would write it now */
static void capture(void)
{
	const int idx = slot_index(twoPlayerMode);
	JE_SaveFileType backup = saveFiles[idx];
	tyGnwNoSaveFile = true;
	JE_saveGame(idx + 1, "GAME & WATCH  ");
	tyGnwNoSaveFile = false;
	snap = saveFiles[idx];
	saveFiles[idx] = backup;
	snap_two = twoPlayerMode;
	snap_valid = true;
}

void ty_state_title(void)
{
	snap_valid = false;
	in_item_screen = false;
}

void ty_state_item_screen(bool entering)
{
	in_item_screen = entering;
	// leaving it to play a level (not to the title screen)
	if (!entering && mainLevel != 0)
		capture();
}

bool ty_core_can_save(void)
{
	return in_item_screen || snap_valid;
}

bool ty_core_save_state(const char *path)
{
	if (in_item_screen)
		capture();
	if (!snap_valid)
		return false;
	ty_state_file_t s;
	memset(&s, 0, sizeof(s));
	s.magic = STATE_MAGIC;
	s.size = sizeof(s);
	s.two_players = snap_two;
	s.save = snap;
	FILE *f = fopen(path, "wb");
	if (!f)
		return false;
	bool ok = fwrite(&s, sizeof(s), 1, f) == 1;
	fclose(f);
	return ok;
}

bool ty_core_load_state(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return false;
	ty_state_file_t s;
	bool ok = fread(&s, sizeof(s), 1, f) == 1 && s.magic == STATE_MAGIC &&
	          s.size == sizeof(s) && s.save.level > 0;
	fclose(f);
	if (!ok)
		return false;
	pending = s;
	pending_valid = true;
	// back to the title screen loop at the next platform service
	restart_requested = restart_ready;
	return true;
}

bool ty_state_pending(void)
{
	return pending_valid;
}

jmp_buf *ty_state_restart_env(void)
{
	restart_ready = true;
	return &restart_env;
}

void ty_state_after_restart(void)
{
	// what the abandoned code was doing
	stop_song();
	keyboardClearInput();
	ESCPressed = false;
	gameLoaded = false;
	fade_song_reset();
}

void ty_core_poll_restart(void)
{
	if (!restart_requested)
		return;
	restart_requested = false;
	longjmp(restart_env, 1);
}

bool ty_state_apply_pending(void)
{
	if (!pending_valid)
		return false;
	pending_valid = false;
	const int idx = slot_index(pending.two_players != 0);
	JE_SaveFileType backup = saveFiles[idx];
	saveFiles[idx] = pending.save;
	performSave = false;
	gameLoaded = false;
	JE_operation(idx + 1);
	saveFiles[idx] = backup;
	return gameLoaded;
}
