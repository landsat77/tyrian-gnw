/*
 * The core of the port: runs OpenTyrian (ot_main(), the main() of
 * src/ot/opentyr.c) on the mini SDL, with the buttons and the sound of the
 * platform.
 */
#include <stdio.h>
#include <string.h>

#include "ty_platform.h"
#include "ty_sdl.h"

#include "episodes.h"
#include "loudness.h"

int ot_main(int argc, char *argv[]);

void ty_exit(int code)
{
	if (code == 0)
		ty_plat_quit();
	char msg[48];
	snprintf(msg, sizeof(msg), "The game stopped (error %d)", code);
	ty_plat_fatal(msg);
}

void ty_core_set_buttons(uint32_t buttons)
{
	ty_sdl_set_buttons(buttons);
}

void ty_core_set_sound(bool music, bool sound)
{
	music_disabled = !music;
	samples_disabled = !sound;
}

void ty_core_audio_render(int16_t *out, int frames)
{
	ty_sdl_audio_render(out, frames);
}

void ty_core_run(const ty_core_options_t *opts)
{
	const size_t weapons_size = sizeof(JE_WeaponType) * (WEAP_NUM + 1);
	weapons = ty_plat_big_memory("weapons", weapons_size);
	if (weapons == NULL)
		weapons = calloc(1, weapons_size);
	if (weapons == NULL)
		ty_plat_fatal("Out of memory (weapons)");

	ty_core_set_sound(opts->music, opts->sound);
	static char arg0[] = "opentyrian";
	char *argv[] = { arg0, NULL };
	ot_main(1, argv);
	ty_exit(0);
}
