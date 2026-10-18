/*
 * Mini SDL 2 of the port (src/port/sdl/SDL.h): 8 bit surfaces, the time,
 * key events made from the buttons of the console, the audio callback.
 *
 * Buttons -> keys (ty_core_set_buttons()). Tyrian is played with the
 * keyboard: in a level (the game reads the player's keys, see
 * ty_sdl_in_action()) A = fire (Space), B = both sidekicks (Left Ctrl +
 * Left Alt), TIME = change the rear weapon mode (Enter), GAME = in-game
 * menu (Escape). In the menus A = Enter, B = Escape, GAME = Escape, the
 * directions repeat while held.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "SDL.h"
#include "ty_platform.h"
#include "ty_sdl.h"

/* ------------------------------------------------------------------ */
/* Misc                                                               */
/* ------------------------------------------------------------------ */
size_t SDL_strlcpy(char *dst, const char *src, size_t size)
{
	size_t n = strlen(src);
	if (size) {
		size_t c = n < size - 1 ? n : size - 1;
		memcpy(dst, src, c);
		dst[c] = '\0';
	}
	return n;
}

/* warnings and errors only (the console prints to its debug output) */
void SDL_LogMessageV(int category, SDL_LogPriority priority, const char *fmt, va_list ap)
{
	(void)category;
	if (priority < SDL_LOG_PRIORITY_WARN)
		return;
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, ap);
	printf("ot: %s\n", buf);
}

void SDL_Log(const char *fmt, ...)
{
	(void)fmt;
}

void SDL_LogDebug(int category, const char *fmt, ...)
{
	(void)category;
	(void)fmt;
}

void SDL_LogWarn(int category, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	SDL_LogMessageV(category, SDL_LOG_PRIORITY_WARN, fmt, ap);
	va_end(ap);
}

void SDL_LogError(int category, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	SDL_LogMessageV(category, SDL_LOG_PRIORITY_ERROR, fmt, ap);
	va_end(ap);
}

/* ------------------------------------------------------------------ */
/* Time                                                               */
/* ------------------------------------------------------------------ */
Uint32 SDL_GetTicks(void)
{
	return ty_plat_time_ms();
}

void SDL_Delay(Uint32 ms)
{
	Uint32 start = ty_plat_time_ms();
	do {
		ty_plat_service(1);
	} while (ty_plat_time_ms() - start < ms);
}

/* ------------------------------------------------------------------ */
/* Surfaces                                                           */
/* ------------------------------------------------------------------ */
static SDL_PixelFormat format8 = { 0, 8, 1 };
static SDL_PixelFormat format888 = { SDL_PIXELFORMAT_RGB888, 32, 4 };

SDL_PixelFormat *SDL_AllocFormat(Uint32 format)
{
	(void)format;
	return &format888;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth,
                                  Uint32 r, Uint32 g, Uint32 b, Uint32 a)
{
	(void)flags; (void)r; (void)g; (void)b; (void)a;
	if (depth != 8)
		return NULL;
	SDL_Surface *s = calloc(1, sizeof(*s));
	if (!s)
		return NULL;
	s->pixels = calloc((size_t)w * h, 1);
	if (!s->pixels) {
		free(s);
		return NULL;
	}
	s->format = &format8;
	s->w = w;
	s->h = h;
	s->pitch = w;
	return s;
}

SDL_Surface *ty_sdl_surface_on(void *pixels, int w, int h, int pitch)
{
	SDL_Surface *s = calloc(1, sizeof(*s));
	if (!s)
		return NULL;
	s->flags = 1; /* pixels not owned */
	s->pixels = pixels;
	s->format = &format8;
	s->w = w;
	s->h = h;
	s->pitch = pitch;
	return s;
}

void SDL_FreeSurface(SDL_Surface *s)
{
	if (!s)
		return;
	if (!(s->flags & 1))
		free(s->pixels);
	free(s);
}

static bool clip(const SDL_Surface *s, SDL_Rect *r)
{
	int x0 = r->x < 0 ? 0 : r->x;
	int y0 = r->y < 0 ? 0 : r->y;
	int x1 = r->x + r->w > s->w ? s->w : r->x + r->w;
	int y1 = r->y + r->h > s->h ? s->h : r->y + r->h;
	if (x1 <= x0 || y1 <= y0)
		return false;
	r->x = x0;
	r->y = y0;
	r->w = x1 - x0;
	r->h = y1 - y0;
	return true;
}

int SDL_FillRect(SDL_Surface *s, const SDL_Rect *rect, Uint32 color)
{
	SDL_Rect r = rect ? *rect : (SDL_Rect){ 0, 0, s->w, s->h };
	if (!clip(s, &r))
		return 0;
	Uint8 *p = (Uint8 *)s->pixels + r.y * s->pitch + r.x;
	for (int y = 0; y < r.h; y++, p += s->pitch)
		memset(p, (int)color, (size_t)r.w);
	return 0;
}

int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr)
{
	SDL_Rect s = sr ? *sr : (SDL_Rect){ 0, 0, src->w, src->h };
	int dx = dr ? dr->x : 0, dy = dr ? dr->y : 0;
	if (!clip(src, &s))
		return 0;
	SDL_Rect d = { dx, dy, s.w, s.h };
	int ox = d.x, oy = d.y;
	if (!clip(dst, &d))
		return 0;
	s.x += d.x - ox;
	s.y += d.y - oy;
	for (int y = 0; y < d.h; y++)
		memmove((Uint8 *)dst->pixels + (d.y + y) * dst->pitch + d.x,
		        (Uint8 *)src->pixels + (s.y + y) * src->pitch + s.x, (size_t)d.w);
	if (dr)
		*dr = d;
	return 0;
}

/* ------------------------------------------------------------------ */
/* Keys                                                               */
/* ------------------------------------------------------------------ */
#define KEY_REPEAT_DELAY 350
#define KEY_REPEAT_RATE  110
#define ACTION_TIMEOUT   150

static uint32_t buttons;
static bool swap_ab;
static bool held[SDL_NUM_SCANCODES];
static SDL_Event queue[32];
static int q_head, q_count;
static uint32_t last_action_ms;
static bool action_seen;
static SDL_Scancode repeat_key;
static uint32_t repeat_at;

void ty_sdl_set_buttons(uint32_t b)
{
	buttons = b;
}

void ty_sdl_swap_ab(bool swap)
{
	swap_ab = swap;
}

void ty_sdl_action(void)
{
	last_action_ms = ty_plat_time_ms();
	action_seen = true;
}

bool ty_sdl_in_action(void)
{
	return action_seen && ty_plat_time_ms() - last_action_ms < ACTION_TIMEOUT;
}

int SDL_PushEvent(SDL_Event *e)
{
	if (q_count == (int)(sizeof(queue) / sizeof(queue[0])))
		return 0;
	queue[(q_head + q_count) % (sizeof(queue) / sizeof(queue[0]))] = *e;
	q_count++;
	return 1;
}

static SDL_Keycode keycode_of(SDL_Scancode sc)
{
	if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
		return 'a' + (sc - SDL_SCANCODE_A);
	switch (sc) {
	case SDL_SCANCODE_RETURN: return '\r';
	case SDL_SCANCODE_ESCAPE: return 27;
	case SDL_SCANCODE_SPACE:  return ' ';
	/* SDL 2 gives 0x40000000 | scancode; the game passes the keycode to
	 * toupper() (title screen), which indexes the C library's table with
	 * it on the console: 0, no character */
	default:                  return 0;
	}
}

static void push_key(SDL_Scancode sc, bool down, bool repeat)
{
	SDL_Event e;
	memset(&e, 0, sizeof(e));
	e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
	e.key.state = down ? SDL_PRESSED : SDL_RELEASED;
	e.key.repeat = repeat;
	e.key.keysym.scancode = sc;
	e.key.keysym.sym = keycode_of(sc);
	SDL_PushEvent(&e);
}

/* the keys the buttons hold now */
static void wanted_keys(bool *want)
{
	uint32_t b = buttons;
	if (swap_ab) {
		uint32_t ab = b & (TY_BTN_A | TY_BTN_B);
		b &= ~(TY_BTN_A | TY_BTN_B);
		if (ab & TY_BTN_A) b |= TY_BTN_B;
		if (ab & TY_BTN_B) b |= TY_BTN_A;
	}
	if (b & TY_BTN_LEFT)  want[SDL_SCANCODE_LEFT] = true;
	if (b & TY_BTN_RIGHT) want[SDL_SCANCODE_RIGHT] = true;
	if (b & TY_BTN_UP)    want[SDL_SCANCODE_UP] = true;
	if (b & TY_BTN_DOWN)  want[SDL_SCANCODE_DOWN] = true;
	if (b & TY_BTN_GAME)  want[SDL_SCANCODE_ESCAPE] = true;
	if (ty_sdl_in_action()) {
		if (b & TY_BTN_A)    want[SDL_SCANCODE_SPACE] = true;
		if (b & TY_BTN_B)    want[SDL_SCANCODE_LCTRL] = want[SDL_SCANCODE_LALT] = true;
		if (b & TY_BTN_TIME) want[SDL_SCANCODE_RETURN] = true;
	} else {
		if (b & TY_BTN_A)    want[SDL_SCANCODE_RETURN] = true;
		if (b & TY_BTN_B)    want[SDL_SCANCODE_ESCAPE] = true;
	}
}

static void update_keys(void)
{
	static const SDL_Scancode used[] = {
		SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN,
		SDL_SCANCODE_ESCAPE, SDL_SCANCODE_SPACE, SDL_SCANCODE_LCTRL, SDL_SCANCODE_LALT,
		SDL_SCANCODE_RETURN,
	};
	bool want[SDL_NUM_SCANCODES];
	memset(want, 0, sizeof(want));
	wanted_keys(want);
	uint32_t now = ty_plat_time_ms();
	for (size_t i = 0; i < sizeof(used) / sizeof(used[0]); i++) {
		SDL_Scancode sc = used[i];
		if (want[sc] && !held[sc]) {
			held[sc] = true;
			push_key(sc, true, false);
			if (sc == SDL_SCANCODE_LEFT || sc == SDL_SCANCODE_RIGHT ||
			    sc == SDL_SCANCODE_UP || sc == SDL_SCANCODE_DOWN) {
				repeat_key = sc;
				repeat_at = now + KEY_REPEAT_DELAY;
			}
		} else if (!want[sc] && held[sc]) {
			held[sc] = false;
			push_key(sc, false, false);
			if (repeat_key == sc)
				repeat_key = SDL_SCANCODE_UNKNOWN;
		}
	}
	/* key repeat of the directions in the menus */
	if (repeat_key != SDL_SCANCODE_UNKNOWN && held[repeat_key] && (int32_t)(now - repeat_at) >= 0) {
		repeat_at = now + KEY_REPEAT_RATE;
		if (!ty_sdl_in_action())
			push_key(repeat_key, true, true);
	}
}

int SDL_PollEvent(SDL_Event *e)
{
	if (q_count == 0) {
		ty_plat_service(0);
		update_keys();
	}
	if (q_count == 0)
		return 0;
	if (e)
		*e = queue[q_head];
	q_head = (q_head + 1) % (int)(sizeof(queue) / sizeof(queue[0]));
	q_count--;
	return 1;
}

static const struct { SDL_Scancode sc; const char *name; } key_names[] = {
	{ SDL_SCANCODE_UP, "Up" }, { SDL_SCANCODE_DOWN, "Down" },
	{ SDL_SCANCODE_LEFT, "Left" }, { SDL_SCANCODE_RIGHT, "Right" },
	{ SDL_SCANCODE_SPACE, "Space" }, { SDL_SCANCODE_RETURN, "Return" },
	{ SDL_SCANCODE_LCTRL, "Left Ctrl" }, { SDL_SCANCODE_LALT, "Left Alt" },
	{ SDL_SCANCODE_ESCAPE, "Escape" },
};

const char *SDL_GetScancodeName(SDL_Scancode sc)
{
	for (size_t i = 0; i < sizeof(key_names) / sizeof(key_names[0]); i++)
		if (key_names[i].sc == sc)
			return key_names[i].name;
	return "";
}

SDL_Scancode SDL_GetScancodeFromName(const char *name)
{
	for (size_t i = 0; i < sizeof(key_names) / sizeof(key_names[0]); i++)
		if (!strcmp(key_names[i].name, name))
			return key_names[i].sc;
	return SDL_SCANCODE_UNKNOWN;
}

/* ------------------------------------------------------------------ */
/* Audio                                                              */
/* ------------------------------------------------------------------ */
static SDL_AudioSpec audio_spec;
static bool audio_open, audio_paused = true;

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *dev, int capture, const SDL_AudioSpec *want,
                                      SDL_AudioSpec *have, int changes)
{
	(void)dev; (void)capture; (void)changes;
	audio_spec = *want;
	audio_spec.freq = ty_plat_audio_rate();
	audio_spec.format = AUDIO_S16SYS;
	audio_spec.channels = 1;
	if (have)
		*have = audio_spec;
	audio_open = true;
	return 1;
}

void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int pause)
{
	(void)d;
	audio_paused = pause != 0;
}

void ty_sdl_audio_render(int16_t *out, int frames)
{
	if (!audio_open || audio_paused || !audio_spec.callback) {
		memset(out, 0, (size_t)frames * sizeof(int16_t));
		return;
	}
	audio_spec.callback(audio_spec.userdata, (Uint8 *)out, frames * (int)sizeof(int16_t));
}
