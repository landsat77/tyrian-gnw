/*
 * Mini SDL 2 for OpenTyrian on the Game & Watch (src/port/ty_sdl.c).
 *
 * Only what OpenTyrian uses: 8 bit software surfaces, key events made from
 * the buttons of the console, the time, the audio callback. Windows,
 * renderers, textures, joysticks and the mouse are stubs (video.c has its
 * own GNW branch).
 */
#ifndef TY_MINI_SDL_H
#define TY_MINI_SDL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "SDL_types.h"
#include "SDL_endian.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_VERSION_ATLEAST(x, y, z) ((x) < 2 || ((x) == 2 && (y) <= 0))

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

#define SDL_PRINTF_FORMAT_STRING
#define SDL_PRINTF_VARARG_FUNC(n) __attribute__((format(printf, n, n + 1)))

/* ---- init / errors / misc ---- */
#define SDL_INIT_AUDIO    0x10u
#define SDL_INIT_VIDEO    0x20u
#define SDL_INIT_JOYSTICK 0x200u

static inline int SDL_Init(Uint32 f) { (void)f; return 0; }
static inline int SDL_InitSubSystem(Uint32 f) { (void)f; return 0; }
static inline void SDL_QuitSubSystem(Uint32 f) { (void)f; }
static inline void SDL_Quit(void) {}
static inline const char *SDL_GetError(void) { return "unsupported"; }
static inline char *SDL_GetBasePath(void) { return NULL; }
static inline void SDL_free(void *p) { free(p); }
static inline bool SDL_SetHint(const char *n, const char *v) { (void)n; (void)v; return true; }
size_t SDL_strlcpy(char *dst, const char *src, size_t size);
#define SDL_vsnprintf vsnprintf

#define SDL_MESSAGEBOX_ERROR 0x10
static inline int SDL_ShowSimpleMessageBox(Uint32 f, const char *t, const char *m, void *w)
{
	(void)f; (void)w;
	printf("%s: %s\n", t, m);
	return 0;
}

/* ---- logging ---- */
enum { SDL_LOG_CATEGORY_APPLICATION = 0 };
typedef enum {
	SDL_LOG_PRIORITY_VERBOSE = 1,
	SDL_LOG_PRIORITY_DEBUG,
	SDL_LOG_PRIORITY_INFO,
	SDL_LOG_PRIORITY_WARN,
	SDL_LOG_PRIORITY_ERROR,
	SDL_LOG_PRIORITY_CRITICAL,
} SDL_LogPriority;
void SDL_LogMessageV(int category, SDL_LogPriority priority, const char *fmt, va_list ap);
void SDL_Log(const char *fmt, ...);
void SDL_LogDebug(int category, const char *fmt, ...);
void SDL_LogWarn(int category, const char *fmt, ...);
void SDL_LogError(int category, const char *fmt, ...);
static inline void SDL_LogSetPriority(int c, SDL_LogPriority p) { (void)c; (void)p; }

/* ---- time ---- */
Uint32 SDL_GetTicks(void);
void SDL_Delay(Uint32 ms);

/* ---- surfaces (8 bit only) ---- */
typedef struct SDL_Color { Uint8 r, g, b, a; } SDL_Color;
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;

#define SDL_PIXELFORMAT_RGB565 0x15151002u
#define SDL_PIXELFORMAT_RGB888 0x16161804u

typedef struct SDL_PixelFormat {
	Uint32 format;
	Uint8 BitsPerPixel;
	Uint8 BytesPerPixel;
} SDL_PixelFormat;

typedef struct SDL_Surface {
	Uint32 flags;
	SDL_PixelFormat *format;
	int w, h;
	int pitch;
	void *pixels;
} SDL_Surface;

#define SDL_MUSTLOCK(s) 0

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth,
                                  Uint32 r, Uint32 g, Uint32 b, Uint32 a);
void SDL_FreeSurface(SDL_Surface *s);
int SDL_FillRect(SDL_Surface *s, const SDL_Rect *r, Uint32 color);
int SDL_BlitSurface(SDL_Surface *src, const SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr);
SDL_PixelFormat *SDL_AllocFormat(Uint32 format);
static inline void SDL_FreeFormat(SDL_PixelFormat *f) { (void)f; }
static inline const char *SDL_GetPixelFormatName(Uint32 f) { (void)f; return "RGB888"; }
/* 0x00RRGGBB (the CLUT of the LCD in LUT8 mode) */
static inline Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b)
{
	(void)f;
	return ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}

/* ---- window / renderer / texture: stubs ---- */
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
#define SDL_WINDOWPOS_CENTERED 0x2FFF0000u
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x1001u
#define SDL_WINDOW_RESIZABLE 0x20u
#define SDL_WINDOW_HIDDEN 0x8u
#define SDL_TEXTUREACCESS_STREAMING 1
static inline SDL_Window *SDL_CreateWindow(const char *t, int x, int y, int w, int h, Uint32 f)
{ (void)t; (void)x; (void)y; (void)w; (void)h; (void)f; return (SDL_Window *)1; }
static inline void SDL_DestroyWindow(SDL_Window *w) { (void)w; }
static inline void SDL_ShowWindow(SDL_Window *w) { (void)w; }
static inline void SDL_GetWindowSize(SDL_Window *w, int *x, int *y) { (void)w; if (x) *x = 320; if (y) *y = 240; }
static inline void SDL_SetWindowSize(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
static inline void SDL_SetWindowPosition(SDL_Window *w, int x, int y) { (void)w; (void)x; (void)y; }
static inline int SDL_SetWindowFullscreen(SDL_Window *w, Uint32 f) { (void)w; (void)f; return 0; }
static inline int SDL_GetWindowDisplayIndex(SDL_Window *w) { (void)w; return 0; }
static inline int SDL_GetNumVideoDisplays(void) { return 1; }
static inline int SDL_GetDisplayBounds(int i, SDL_Rect *r) { (void)i; r->x = r->y = 0; r->w = 320; r->h = 240; return 0; }

/* ---- events ---- */
typedef enum {
	SDL_SCANCODE_UNKNOWN = 0,
	SDL_SCANCODE_A = 4, SDL_SCANCODE_B, SDL_SCANCODE_C, SDL_SCANCODE_D, SDL_SCANCODE_E,
	SDL_SCANCODE_F, SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_I, SDL_SCANCODE_J,
	SDL_SCANCODE_K, SDL_SCANCODE_L, SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O,
	SDL_SCANCODE_P, SDL_SCANCODE_Q, SDL_SCANCODE_R, SDL_SCANCODE_S, SDL_SCANCODE_T,
	SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X, SDL_SCANCODE_Y,
	SDL_SCANCODE_Z,
	SDL_SCANCODE_1 = 30, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5,
	SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_0,
	SDL_SCANCODE_RETURN = 40, SDL_SCANCODE_ESCAPE = 41, SDL_SCANCODE_BACKSPACE = 42,
	SDL_SCANCODE_TAB = 43, SDL_SCANCODE_SPACE = 44, SDL_SCANCODE_MINUS = 45,
	SDL_SCANCODE_EQUALS = 46, SDL_SCANCODE_LEFTBRACKET = 47, SDL_SCANCODE_RIGHTBRACKET = 48,
	SDL_SCANCODE_BACKSLASH = 49, SDL_SCANCODE_SEMICOLON = 51, SDL_SCANCODE_GRAVE = 53,
	SDL_SCANCODE_COMMA = 54, SDL_SCANCODE_PERIOD = 55, SDL_SCANCODE_SLASH = 56,
	SDL_SCANCODE_CAPSLOCK = 57,
	SDL_SCANCODE_F1 = 58, SDL_SCANCODE_F2, SDL_SCANCODE_F3, SDL_SCANCODE_F4, SDL_SCANCODE_F5,
	SDL_SCANCODE_F6, SDL_SCANCODE_F7, SDL_SCANCODE_F8, SDL_SCANCODE_F9, SDL_SCANCODE_F10,
	SDL_SCANCODE_F11, SDL_SCANCODE_F12,
	SDL_SCANCODE_SCROLLLOCK = 71, SDL_SCANCODE_INSERT = 73, SDL_SCANCODE_HOME = 74,
	SDL_SCANCODE_PAGEUP = 75, SDL_SCANCODE_DELETE = 76, SDL_SCANCODE_END = 77,
	SDL_SCANCODE_PAGEDOWN = 78, SDL_SCANCODE_RIGHT = 79, SDL_SCANCODE_LEFT = 80,
	SDL_SCANCODE_DOWN = 81, SDL_SCANCODE_UP = 82, SDL_SCANCODE_NUMLOCKCLEAR = 83,
	SDL_SCANCODE_KP_ENTER = 88,
	SDL_SCANCODE_KP_1 = 89, SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_3, SDL_SCANCODE_KP_4,
	SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_7, SDL_SCANCODE_KP_8,
	SDL_SCANCODE_KP_9, SDL_SCANCODE_KP_0,
	SDL_SCANCODE_LCTRL = 224, SDL_SCANCODE_LSHIFT = 225, SDL_SCANCODE_LALT = 226,
	SDL_SCANCODE_RCTRL = 228, SDL_SCANCODE_RSHIFT = 229, SDL_SCANCODE_RALT = 230,
	SDL_NUM_SCANCODES = 512
} SDL_Scancode;

typedef Sint32 SDL_Keycode;
enum {
	SDLK_RIGHTBRACKET = ']',
	SDLK_d = 'd', SDLK_g = 'g', SDLK_l = 'l', SDLK_o = 'o', SDLK_r = 'r', SDLK_s = 's',
};

typedef enum {
	KMOD_NONE = 0,
	KMOD_LSHIFT = 0x1, KMOD_RSHIFT = 0x2,
	KMOD_LCTRL = 0x40, KMOD_RCTRL = 0x80,
	KMOD_LALT = 0x100, KMOD_RALT = 0x200,
	KMOD_LGUI = 0x400, KMOD_RGUI = 0x800,
} SDL_Keymod;
#define KMOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define KMOD_CTRL  (KMOD_LCTRL | KMOD_RCTRL)
#define KMOD_ALT   (KMOD_LALT | KMOD_RALT)
#define KMOD_GUI   (KMOD_LGUI | KMOD_RGUI)

typedef struct SDL_Keysym {
	SDL_Scancode scancode;
	SDL_Keycode sym;
	Uint16 mod;
} SDL_Keysym;

enum {
	SDL_QUIT = 0x100,
	SDL_WINDOWEVENT = 0x200,
	SDL_KEYDOWN = 0x300,
	SDL_KEYUP,
	SDL_TEXTINPUT = 0x303,
	SDL_MOUSEMOTION = 0x400,
	SDL_MOUSEBUTTONDOWN,
	SDL_MOUSEBUTTONUP,
};
enum {
	SDL_WINDOWEVENT_RESIZED = 5,
	SDL_WINDOWEVENT_FOCUS_GAINED = 12,
	SDL_WINDOWEVENT_FOCUS_LOST = 13,
};
#define SDL_RELEASED 0
#define SDL_PRESSED 1

typedef struct { Uint32 type; Uint8 event; } SDL_WindowEvent;
typedef struct { Uint32 type; Uint8 state; Uint8 repeat; SDL_Keysym keysym; } SDL_KeyboardEvent;
typedef struct { Uint32 type; char text[32]; } SDL_TextInputEvent;
typedef struct { Uint32 type; Sint32 x, y, xrel, yrel; } SDL_MouseMotionEvent;
typedef struct { Uint32 type; Uint8 button; Sint32 x, y; } SDL_MouseButtonEvent;

typedef union SDL_Event {
	Uint32 type;
	SDL_WindowEvent window;
	SDL_KeyboardEvent key;
	SDL_TextInputEvent text;
	SDL_MouseMotionEvent motion;
	SDL_MouseButtonEvent button;
} SDL_Event;

int SDL_PollEvent(SDL_Event *e);
int SDL_PushEvent(SDL_Event *e);
static inline void SDL_StartTextInput(void) {}
static inline void SDL_StopTextInput(void) {}
const char *SDL_GetScancodeName(SDL_Scancode sc);
SDL_Scancode SDL_GetScancodeFromName(const char *name);

/* ---- mouse: none ---- */
#define SDL_BUTTON(x) (1 << ((x) - 1))
#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON_LMASK SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_ENABLE 1
#define SDL_DISABLE 0
#define SDL_IGNORE 0
#define SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE "SDL_MOUSE_RELATIVE_SYSTEM_SCALE"
static inline int SDL_ShowCursor(int t) { (void)t; return 0; }
static inline int SDL_SetRelativeMouseMode(SDL_bool e) { (void)e; return 0; }

/* ---- joysticks: none ---- */
typedef struct SDL_Joystick SDL_Joystick;
#define SDL_HAT_CENTERED 0
#define SDL_HAT_UP 1
#define SDL_HAT_RIGHT 2
#define SDL_HAT_DOWN 4
#define SDL_HAT_LEFT 8
static inline int SDL_NumJoysticks(void) { return 0; }
static inline int SDL_JoystickEventState(int s) { (void)s; return 0; }
static inline SDL_Joystick *SDL_JoystickOpen(int i) { (void)i; return NULL; }
static inline void SDL_JoystickClose(SDL_Joystick *j) { (void)j; }
static inline void SDL_JoystickUpdate(void) {}
static inline const char *SDL_JoystickName(SDL_Joystick *j) { (void)j; return ""; }
static inline int SDL_JoystickNumAxes(SDL_Joystick *j) { (void)j; return 0; }
static inline int SDL_JoystickNumButtons(SDL_Joystick *j) { (void)j; return 0; }
static inline int SDL_JoystickNumHats(SDL_Joystick *j) { (void)j; return 0; }
static inline Sint16 SDL_JoystickGetAxis(SDL_Joystick *j, int i) { (void)j; (void)i; return 0; }
static inline Uint8 SDL_JoystickGetButton(SDL_Joystick *j, int i) { (void)j; (void)i; return 0; }
static inline Uint8 SDL_JoystickGetHat(SDL_Joystick *j, int i) { (void)j; (void)i; return 0; }

/* ---- audio: one callback, pulled by the platform ---- */
typedef Uint16 SDL_AudioFormat;
#define AUDIO_S8     0x8008
#define AUDIO_S16SYS 0x8010
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE 0x1
#define SDL_AUDIO_ALLOW_SAMPLES_CHANGE 0x8
typedef void (*SDL_AudioCallback)(void *userdata, Uint8 *stream, int len);
typedef struct SDL_AudioSpec {
	int freq;
	SDL_AudioFormat format;
	Uint8 channels;
	Uint8 silence;
	Uint16 samples;
	Uint32 size;
	SDL_AudioCallback callback;
	void *userdata;
} SDL_AudioSpec;
typedef Uint32 SDL_AudioDeviceID;
SDL_AudioDeviceID SDL_OpenAudioDevice(const char *dev, int capture, const SDL_AudioSpec *want,
                                      SDL_AudioSpec *have, int changes);
void SDL_PauseAudioDevice(SDL_AudioDeviceID d, int pause);
static inline void SDL_CloseAudioDevice(SDL_AudioDeviceID d) { (void)d; }
static inline void SDL_LockAudioDevice(SDL_AudioDeviceID d) { (void)d; }
static inline void SDL_UnlockAudioDevice(SDL_AudioDeviceID d) { (void)d; }

typedef struct SDL_AudioCVT {
	Uint8 *buf;
	int len;
	int len_cvt;
	int len_mult;
} SDL_AudioCVT;
static inline int SDL_BuildAudioCVT(SDL_AudioCVT *c, SDL_AudioFormat sf, Uint8 sc, int sr,
                                    SDL_AudioFormat df, Uint8 dc, int dr)
{ (void)sf; (void)sc; (void)sr; (void)df; (void)dc; (void)dr; c->len_mult = 1; return -1; }
static inline int SDL_ConvertAudio(SDL_AudioCVT *c) { (void)c; return -1; }

/* ---- port: the mini SDL and the platform (ty_sdl.c) ---- */
SDL_Surface *ty_sdl_surface_on(void *pixels, int w, int h, int pitch);

#ifdef __cplusplus
}
#endif

#endif
