/*
 * Video of OpenTyrian on the Game & Watch (replaces src/ot/video.c and
 * video_scale.c): the game draws into 320x200 8 bit surfaces, JE_showVGA()
 * gives VGAScreen and its palette (rgb_palette, 0x00RRGGBB, made by
 * palette.c with SDL_MapRGB()) to the platform.
 *
 * The scaling mode of the setup menu chooses the picture on the 320x240
 * LCD: "Original" (centered, black bars) or "Full Screen" (stretched).
 */
#include "video.h"

#include "palette.h"
#include "video_scale.h"

#include <string.h>

#include "ty_platform.h"

/* the setup menu offers the first two (opentyr.c GNW) */
const char *const scaling_mode_names[ScalingMode_MAX] = {
	"Original",
	"Full Screen",
	"Fit 8:5",
	"Fit 4:3",
};

int fullscreen_display = -1;
ScalingMode scaling_mode = SCALE_INTEGER;  // "Full Screen"

SDL_Surface *VGAScreen, *VGAScreenSeg;
SDL_Surface *VGAScreen2;
SDL_Surface *game_screen;

SDL_Window *main_window = NULL;
SDL_PixelFormat *main_window_tex_format = NULL;

/* one "scaler": the LCD */
uint scaler = 0;
const struct Scalers scalers[] = {
	{ 320, 200, NULL, NULL, "None" },
};
const uint scalers_count = 1;

void set_scaler_by_name(const char *name)
{
	(void)name;
}

/* palette.c changes rgb_palette: the platform converts again when this
 * changes (cheap test: a checksum of the palette) */
static uint32_t palette_version(void)
{
	uint32_t h = 2166136261u;
	for (int i = 0; i < 256; i++)
		h = (h ^ rgb_palette[i]) * 16777619u;
	return h;
}

/* a 320x200 screen, outside the heap if the platform can */
static SDL_Surface *create_screen(const char *what)
{
	void *pixels = ty_plat_big_memory(what, vga_width * vga_height);
	if (pixels != NULL)
		return ty_sdl_surface_on(pixels, vga_width, vga_height, vga_width);
	return SDL_CreateRGBSurface(0, vga_width, vga_height, 8, 0, 0, 0, 0);
}

void init_video(void)
{
	main_window_tex_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);

	VGAScreen = VGAScreenSeg = create_screen("screen0");
	VGAScreen2 = create_screen("screen1");
	game_screen = create_screen("screen2");
	if (!VGAScreen || !VGAScreen2 || !game_screen)
		ty_plat_fatal("Out of memory (screens)");

	JE_clr256(VGAScreen);
}

void deinit_video(void)
{
}

void video_on_win_resize(void)
{
}

void reinit_fullscreen(int new_display)
{
	fullscreen_display = new_display;
}

void toggle_fullscreen(void)
{
}

bool init_scaler(unsigned int new_scaler)
{
	scaler = new_scaler < scalers_count ? new_scaler : 0;
	return true;
}

bool set_scaling_mode_by_name(const char *name)
{
	for (int i = 0; i < ScalingMode_MAX; ++i) {
		if (strcmp(name, scaling_mode_names[i]) == 0) {
			scaling_mode = i;
			return true;
		}
	}
	return false;
}

bool ty_video_stretch(void)
{
	return scaling_mode != SCALE_CENTER;
}

void JE_clr256(SDL_Surface *screen)
{
	SDL_FillRect(screen, NULL, 0);
}

void JE_showVGA(void)
{
	ty_plat_present(VGAScreen->pixels, VGAScreen->pitch, rgb_palette, palette_version());
}

void mapScreenPointToWindow(Sint32 *inout_x, Sint32 *inout_y)
{
	(void)inout_x;
	(void)inout_y;
}

void mapWindowPointToScreen(Sint32 *inout_x, Sint32 *inout_y)
{
	(void)inout_x;
	(void)inout_y;
}

void scaleWindowDistanceToScreen(Sint32 *inout_x, Sint32 *inout_y)
{
	*inout_x = 0;
	*inout_y = 0;
}
