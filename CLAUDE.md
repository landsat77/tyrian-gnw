# CLAUDE.md — Tyrian homebrew (Retro-Go SD)

This repo is ONE GWHB homebrew: Tyrian 2.1 (OpenTyrian) for the Game &
Watch, built with the retro-go-sd-templates SDK. The second part of this file
is the SDK's generic porting guide (memory map, audio, ABI).

## Project notes

- Target firmware: Retro-Go SD v2.0.0-rc1+ (ABI v2). Homebrews live in
  `/homebrews/` (SD root): `Tyrian.bin` + `Tyrian.ty` (tools/ty_pack.py from
  the freeware tyrian21.zip; names in lower case, music.mus replaced by the
  rendered musicNN.mus).
- Engine: `src/ot` = OpenTyrian (commercial-game-sources/opentyrian @
  5a9d8da, GPLv2), C. Every change is under `#ifdef GNW`. Replaced files:
  file.c (ty_file.c: pack in place + user files in the save dir), video.c /
  video_scale*.c (ty_video.c), loudness.c (ty_loudness.c). Not built on the
  console: lds_play.c / opl.c (only the music renderer). network.c is built
  without WITH_NETWORK. `main` is `ot_main` (ty_core.c calls it). Build
  with -DNDEBUG (intro logos). Glue `gnw_ty_glue.h`: malloc & co -> ty heap,
  exit -> ty_exit (0: quit to the launcher, else fatal), atexit ignored.
- Mini SDL 2 (`src/port/sdl/SDL.h`, `ty_sdl.c`): 8 bit surfaces, ticks,
  SDL_Delay/PollEvent -> ty_plat_service(), key events made from the
  buttons. Context: in a level (mainint.c calls ty_sdl_action() where it
  reads the player's keys, valid 150 ms) A=Space fire, B=LCtrl+LAlt
  sidekicks, TIME=Enter rear mode, GAME=Esc; in the menus A=Enter, B=Esc,
  directions repeat (350 / 110 ms). No mouse (has_mouse false), no joystick,
  windowHasFocus = true (else NDEBUG pauses the game).
  Keycodes: letters / Enter / Esc / Space only, 0 for the other keys (the
  title screen does toupper(sym): SDL's 0x40000000|scancode indexed
  newlib's ctype table out of range -> Hardfault on the console, v1.0.1;
  gwemu did not fault).
- Display: LCD in LUT8 mode. SDL_MapRGB returns 0x00RRGGBB (rgb_palette of
  palette.c) -> lcd_set_clut() when the palette changes (FNV hash as
  version; forced after the pause menu). 320x200 copied line by line,
  stretched to 240 (setup "Picture: Full Screen", scaling_mode !=
  SCALE_CENTER) or centered ("Original").
- Memory (RAM_EMU: code 186 KB + BSS 343 KB, heap ~205 KB, peak ~126 KB):
  the 3 screens are outside the heap (ty_plat_big_memory: screen0 = ITCM,
  +256 bytes because ITCM starts at address 0; screen1/2 = LUT8 bonus
  pool), `weapons` (62 KB, a pointer under GNW) in DTCM. Sprites (sprite.c
  load_sprites / JE_loadCompShapesB), level tiles (tyrian2.c: mainmap points
  into shapesX.dat; megaData*.shapes removed) and samples are used in place
  from the pack. Sprite sheets sit at odd addresses: offsets read byte by
  byte (sprite2_offset).
- Sound: ty_loudness.c mixes the 8 bit 11025 Hz samples in place (16.16
  step) and the musicNN.mus pieces (ty_music.c, GWMU v2; codec bit 15 =
  the song ends, playing=false at its end; fade_song = Loudness fade, one
  255th per 69.5 Hz update). Volumes as loudness.c (30 dB table without
  powf, music x2). Ring of the firmware (pcm_attach), 2048 samples ahead.
- Music rendering (ty_render.c, harness -R): Loudness player at 69.5 Hz +
  OPL at 22050, decimated to 11025 (63 tap sinc). Loop = jump back of the
  player (songlooped): the second pass is searched within +-0.5 s of the
  jump (best rms vs the first pass), file = pass 1 + 2 s, loop start =
  position start + 2 s. Songs that stop (0xFC) end with 0.5 s of release.
  Deterministic on every PC (checked gcc / zig musl / Windows under wine):
  GNW xorshift instead of rand() in opl.c, empty instrument for numbers
  past the bank (lds_playsound read garbage), wrapped product in
  operator_advance (signed overflow).
- Savestates (ty_state.c): a Tyrian save record (JE_saveGame into a temp
  slot, tyGnwNoSaveFile: no tyrian.sav write) captured when the game menu
  (JE_itemScreen) is left for a level, or live in the game menu. Load =
  pending; boot: no intro, titleScreen() loads it (JE_operation) -> game
  menu. While running: ty_core_poll_restart() after the platform service
  longjmps to ot_main (setjmp before its title loop).
- Files: data in the pack, user files (tyrian.cfg, tyrian.sav,
  opentyrian.cfg) in /data/tyrian/<stem>/.
- Harness: `make -C harness` -> harness/build/ty_host (virtual clock,
  scripts, PPM/WAV, -q/-Q, -j/-J, -L, -R/-M/-A, -H; SAN=1). Big buffers
  come from calloc (outside the heap) like on the console: -H 200 matches
  it. `tools/build_renderers.sh` builds ty_render for Windows/macOS/Linux.
- Device test: gwemu (fw-rc1) with QMP; keys x = A, z = B, esc = PAUSE/SET,
  ret = GAME. Level 1: 35 fps (the game's speed), idle ~88 %.

---

## SDK guide — porting an emulator core (Retro-Go SD)

This repository is a **single-project** template for a Game & Watch
**dynamic core** or a **GWHB homebrew**: a freestanding Cortex-M7 binary
that the firmware loads into a fixed RAM window and talks to **only**
through `gw_firmware_abi_t`. You do not link against the firmware ELF.

One tree = one binary. Choose the kind at build time:


| Kind         | `PROJECT_KIND` | Packer             | SD path                |
| ------------ | -------------- | ------------------ | ---------------------- |
| Dynamic core | `core` (default) | `pack_core.py`   | `/cores/*.bin`         |
| Homebrew     | `homebrew`     | `pack_homebrew.py` | `/homebrews/*.bin` |


Read `README.md` for build/pack basics. Read `sdk/ld/core_ram_emu.ld` for
the default linker contract. This file is the memory + porting checklist.

## Mental model

```
SD /cores/<name>.bin          (CORE)     → emulators_scan_cores() → system tab
SD /homebrews/<name>.bin      (GWHB)     → Homebrew tab → run_gwhb_homebrew()
  → memcpy code → RAM_EMU, zero BSS, jump Thumb entry
  → your CORE_ENTRY loop calls LCD / audio / FS / input via the ABI bridge
```

- One binary at a time owns the emulator RAM window.
- No crt0 shared with the launcher; undefined symbols must go through
`sdk/src/gw_core_bridge.c` + `gw_core_bridge_redefine_syms.txt`.
- Pack with `make` / `make PROJECT_KIND=homebrew` (root Makefile calls
`pack_core.py` or `pack_homebrew.py`).



## Porting checklist (cores)

1. Start from this repo (`PROJECT_KIND=core`).
2. Set `CORE_NAME`, `CORE_ENTRY`, `CORE_C_SOURCES` in the root `Makefile`
  (it already `include`s `sdk/Makefile`).
3. In `src/main.c`, include firmware-style headers first, then
  `#include "gw_core_bridge.h"` **last** (macros rewrite `ACTIVE_FILE` /
   `common_emu_state`).
4. Wire `odroid_system_init` + `odroid_system_emu_init` (save/load/screenshot
   hooks as needed). Pass `APPID_CORE` or `APPID_HOMEBREW` from
   `Core/Inc/retro-go/appid.h` (synced with the firmware — not a per-system id).
5. Frame loop pattern (see `src/main.c`):
  - `wdog_refresh()` regularly (WWDG is ~hundreds of ms — a slow frame or
   a large memset without kicks soft-resets with no useful log).
  - `common_emu_frame_loop()` → input → emulate → present → audio.
6. Fix link errors by adding **both** a redefine-syms line and a `core_`*
  trampoline. If the symbol is not on the ABI yet, extend the **firmware**
   first, then `./scripts/sync_from_firmware.sh <firmware-tree>`.
7. Match `required_abi_version` / `required_abi_min_size` to the firmware
  you flash (`SDK_VERSION` records the sync).
8. Pack logos from `src/assets/*.bmp|png` via `--pad-logo` / `--header-logo`
  (dark-on-light). Optional: `--cheat-ext ggcodes|pceplus|mcf` (or
   `cheat_ext=` in `--system`) so the launcher only probes that cheat
   file type — leave empty if unsupported.



## Porting checklist (homebrews)

Same as cores for steps 2–7, but:

1. Build with `PROJECT_KIND=homebrew` (same `src/main.c`; ROM/cheat paths are
   compiled out via `PROJECT_KIND_HOMEBREW`).
2. Pack metadata uses `pack_homebrew.py` (`--name`, `--version`, optional
   `--cover`) — see the homebrew branch in the root `Makefile`. Extra
   segments (ITCM / RAM_UC) use the same `CORE_EXTRA_SEGMENTS` / auto
   ELF discovery as cores (`gnw_core_segment_t` in the GWHB header).
3. No ROM load — `ACTIVE_FILE` is the GWHB `.bin` under `/roms/homebrew/`.
4. Cover JPEG ≤ 186×100 and ≤ 10 KiB; `/covers/homebrew/<stem>.img`
  overrides the embedded cover if present.
5. Sidecar assets that do not fit in RAM_EMU stay as sibling SD files.



### Include / link gotchas

- `objcopy --redefine-syms` rejects **blank lines** in the map file.
- `__aeabi_memset` / `__aeabi_memclr` take `(dest, n, c)` — `n` and `c`
swapped vs libc `memset`. Use the bridge trampolines; do not alias them
to `memset`.
- newlib macros (`isalnum`, `feof`, …): call through ABI/bridge wrappers
already provided, or you will pull in unwanted libc.
- Hard-float `fpv5-d16` is mandatory (same calling convention as firmware).

---



## Memory map (STM32H7B0, SD Retro-Go)

Addresses and lengths must match the firmware linker scripts vendored under
`sdk/ld/`. If you change a shared `gnw_*.ld`, rebuild **firmware and every
core**.

### Summary table


| Region                     | Approx. base                   | Size (usable)                             | Access                                        | Allocator / how you get it                                                                                                                                                                                                                                                                                                                        | Best for                                                                    |
| -------------------------- | ------------------------------ | ----------------------------------------- | --------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------- |
| **ITCM**                   | `0x00000000`                   | **64 KiB**                                | Zero-wait instruction + data, tightly coupled | `itc_malloc` / `itc_calloc`; and/or packed `GNW_CORE_REGION_ITCM` segment (`gnw_itcm_core.ld`)                                                                                                                                                                                                                                                    | Hot interpreters, opcode dispatch, tiny hot loops — **code first**          |
| **DTCM**                   | `0x20000000`                   | **128 KiB** total; **~104 KiB** for cores | Zero-wait data TCM                            | Stack (~24 KiB + redzone at top). Core pool is the bump below that: `dtcm_init` / `dtcm_malloc` / `dtcm_calloc` (**no** `free`; forgotten by `dtcm_init()`). Plan on **~104 KiB**                                                                                                                                                                 | Small hot **state** (CPU regs, line buffers, tiny synth structs)            |
| **AHB SRAM**               | `0x30000000`                   | **128 KiB** total                         | AXI/AHB SRAM; **no** special I-fetch path     | Top **8 KiB** = firmware `.audio` DMA (non-cacheable). Remainder holds firmware `.persistent` / `.data` / `.bss` then the **newlib heap**. Cores use `malloc` / `calloc` / `free` **or** `ahb_malloc` / `ahb_calloc` (aliases). Budget roughly **~64 KiB − 8 KiB audio ≈ ~56 KiB** freeable heap after firmware AHB usage — exact leftover varies | Medium buffers that must not eat DTCM/RAM_EMU; anything that needs `free()` |
| **AXI SRAM (framebuffer)** | `0x24000000`                   | **300 KiB** (RGB565) / **150 KiB** (LUT8) | Uncached / LCD path                           | Firmware-owned double framebuffer (2×320×240 RGB565 or 1×LUT8). In LUT8 mode the upper **150 KiB** is freed as the cacheable **RAM_UC** window                                                                                                                                                                                                   | LUT8 bonus: `GNW_CORE_REGION_RAM_UC` segment or `lcd_get_bonus_pool()` heap |
| **AXI SRAM (RAM_EMU)**     | after FB (`__RAM_EMU_START__`) | **~724 KiB** (`1024 KiB − 300 KiB`)       | Cached AXI                                    | Core **link** image (`.text` / `.rodata` / `.data` / `.bss`) + `ram_malloc` / `ram_calloc` bump (firmware seeds the pool after load)                                                                                                                                                                                                              | Default home for code, BSS, WRAM, VRAM, frame staging, most emulator state  |


Exact constants: `sdk/ld/gnw_ram_emu.ld`, `gnw_itcm_core.ld`, `gnw_ram_uc_core.ld`.

### Pool APIs (`gw_malloc.h` → `mem_ctl`)


| Call                          | Pool                     | Notes                                                                                                                                                             |
| ----------------------------- | ------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `itc_malloc` / `itc_calloc`   | ITCM bump                | Failure sentinel can be `0xffffffff` (NULL is a valid ITCM address). Call `itc_init` when appropriate. Competes with any ITCM **segment** you pack into the core. |
| `ram_malloc` / `ram_calloc`   | RAM_EMU bump             | Firmware seeds `ram_start` after load. Call `ram_init()` to rewind the bump back to `ram_start`. Only assign `ram_start` yourself if you intentionally move that base before `ram_init()`. `ram_get_free_size()` for leftover. |
| `malloc` / `calloc` / `free`  | **AHB** newlib heap      | Same physical pool as `ahb_`*. Freeable. No pool-wide reset (launcher AHB state must survive).                                                                    |
| `ahb_malloc` / `ahb_calloc`   | **AHB** newlib heap      | Aliases of `malloc` / `calloc`. Prefer these when you want the pool to be obvious in the source. Large clears: chunk + `wdog_refresh()`.                          |
| `dtcm_malloc` / `dtcm_calloc` | **DTCM** bump (~104 KiB) | Fast hot state. **No** `free` — call `dtcm_init()` to rewind. Do not confuse with `malloc`.                                                                       |


There is **no** `dtcm_free` and **no** AHB bump rewind in the current ABI —
AHB is the freeable heap; DTCM is the bump pool. For RAM_EMU, `ram_init()`
is the rewind (back to the current `ram_start`).

### Choosing a home (rules of thumb)

1. **Interpreter / dispatch ASM or hottest** `.c` → ITCM (segment or
  `itc_malloc` for blobs). Cap: 64 KiB **total** for everything in ITCM.
2. **Small struct touched every opcode / every scanline** (CPU state,
  control block, one line buffer) → `dtcm_malloc` / `dtcm_calloc`; keep
   the aggregate under ~104 KiB.
3. **Freeable / medium buffers** (tens of KiB that need `free`, or
  share the general heap) → `malloc` / `ahb_malloc`. Stay within the
   ~56 KiB AHB heap budget.
4. **Large session-lifetime buffers** (sound RAM, cart SRAM, big video
  backing) → RAM_EMU (`ram_malloc` or link BSS) if they will not fit in
   AHB/DTCM.
5. Never assume a failed `ahb_malloc`/`malloc` falls back to RAM_EMU —
  it does not. Log the pointer (`0x30…` = AHB, `0x20…` = DTCM,
   `0x24…` = AXI/RAM_EMU).



### ITCM vs AHB vs RAM_EMU for *code*


|         | I-fetch              | Typical use in cores                                  |
| ------- | -------------------- | ----------------------------------------------------- |
| ITCM    | Fastest              | Thumb-2 / hand ASM interpreters and hottest dispatch  |
| RAM_EMU | Cached AXI           | Most `.text`; secondary engines that did not fit ITCM |
| AHB     | No I-fetch advantage | Prefer **data**; packed AHB code only for capacity    |


You cannot put a ~66 KiB object in 64 KiB ITCM. If ITCM already holds a
large engine, the next hot buffer goes to AHB or RAM_EMU.

### LUT8 mode (8-bit indexed palette)

The LTDC supports two pixel formats at runtime, switchable per-core:

| | RGB565 (default) | LUT8 |
|---|---|---|
| Bytes per pixel | 2 | 1 |
| Framebuffer ×2 | 2 × 150 KiB = **300 KiB** | 2 × 75 KiB = **150 KiB** |
| Bonus pool freed | 0 | **~150 KiB** (cacheable AXI) |
| Max simultaneous colors | 65 536 | 256 (CLUT entries, 0x00RRGGBB) |
| Drawing code | `uint16_t *fb` | `uint8_t *fb` + CLUT indices |

#### When to use LUT8

- The emulated system has a **small palette** (≤ 256 colors): Game Boy,
  NES, PICO-8, Master System, Game Gear, PC Engine (background/sprite
  combined ≤ 256).
- You need the extra **150 KiB RAM_UC** for code/data that won't fit in
  RAM_EMU alone.
- Halving the framebuffer bandwidth can help with DMA timing on tight
  scanline budgets.

#### When NOT to use LUT8

- The emulated system outputs **direct-color or > 256 simultaneous colors**
  (Mega Drive DAC scenes, SNES hi-color, GBA with > 256 entries, etc.).
- The core draws directly in RGB565 and converting to palette indices every
  frame would cost more than it saves.

#### How to switch

```c
#include "gw_lcd.h"

lcd_setup_framebuffers(LCD_MODE_LUT8);
lcd_clear_buffers();

static const uint32_t my_clut[16] = {
    0x000000, 0x1D2B53, 0x7E2553, 0x008751,
    0xAB5236, 0x5F574F, 0xC2C3C7, 0xFFF1E8,
    0xFF004D, 0xFFA300, 0xFFEC27, 0x00E436,
    0x29ADFF, 0x83769C, 0xFF77A8, 0xFFCCAA,
};
lcd_set_clut(my_clut, 16);
```

After `lcd_set_clut(clut, count)`:
- Indices `[0..count)` hold the cart palette.
- Indices `[count..2*count)` hold auto-generated **darkened twins**
  (LCD_DARKEN_BIT = 0x20). A fullscreen darken is just `fb[i] |= 0x20`.
- If `2*count ≤ 256`, the remaining slots are available for the overlay
  theme (`lcd_set_overlay_clut`, auto-registered at `LCD_OVERLAY_CLUT_BASE`
  = 0x40).
- If `count == 256`, darkened twins and overlay share those 256 slots
  (overlay overwrites `[64..]` while the pause menu is drawn).

#### Mode-agnostic drawing

Use `lcd_pen_t` to write pixels without branching on mode everywhere:

```c
lcd_pen_t pen = lcd_pen(0xFFFF); /* white in RGB565 → nearest CLUT idx in LUT8 */
lcd_pen_set(&pen, y * WIDTH + x);
lcd_pen_run(&pen, y * WIDTH + x, count);
lcd_pen_darken(&pen, y * WIDTH + x);
```

Or check once at the top:

```c
int mode_lut8 = (lcd_get_mode() == LCD_MODE_LUT8);
size_t frame_sz = lcd_get_frame_size(); /* 153600 or 76800 */
```

#### Bonus pool (RAM_UC)

In LUT8 mode, the **upper 150 KiB** of the LCD pool becomes available:

```c
uint8_t *pool; size_t pool_size;
lcd_get_bonus_pool(&pool, &pool_size);
```

This memory starts at `__RAM_UC_CORE_START__` and is cacheable AXI SRAM.
If the core has a `GNW_CORE_REGION_RAM_UC` segment (packed via
`gnw_ram_uc_core.ld`), the loader places code/data there first, and
`lcd_get_bonus_pool` returns whatever is left after that segment.

#### Constraints

- CLUT programming (`lcd_set_clut`) takes effect at the next LTDC
  refresh — do it during vblank or at init, not mid-scanline.
- `GW_LCD_FRAME_SIZE` is a **compile-time** constant (RGB565). For
  runtime-safe framebuffer sizing use `lcd_get_frame_size()`.
- Switching back to RGB565 (`lcd_setup_framebuffers(LCD_MODE_RGB565)`)
  reclaims the full 300 KiB for framebuffers and invalidates the bonus
  pool pointer. Clear framebuffers after switching.
- The bonus pool is **not** available in RGB565 mode.

### Watchdog and big clears

The window watchdog will soft-reset the device if not fed. Patterns that
have bitten ports:

- 64 KiB+ `memset` without chunking / `wdog_refresh`
- Per-cycle audio catch-up for a full frame with no kick
- Deep `printf` on a nearly full stack (corrupts spilled pointers)

Prefer: refresh every few KiB of clear, every scanline in the emu loop,
and between audio catch-up chunks.

### Cache / DMA

- LCD and audio DMA are bus masters and **cache-blind**. Clean D-cache
before DMA2D/SAI reads core-produced buffers in cacheable RAM.
- Scaled present may use ABI `dma2d_ctl` — see firmware comments; poll
with watchdog slices, do not block for 100 ms unfed.

---



## Audio

Firmware owns the SAI/DMA path and the double buffer in AHB
(`audiobuffer_dma` in `.audio`, top **8 KiB** of AHB — non-cacheable).
Cores never allocate that buffer; they fill the **active half** each
period and pace on DMA.

### Contract


| Piece                                                     | Role                                                                                                                                                    |
| --------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `odroid_system_init(app_id, sample_rate)`                 | Sets the core sample rate (also reaches `audio_ctl` / `GW_AUDIO_INIT`). Must be one of the rates below.                                                 |
| `audio_start_playing(n)`                                  | Start DMA; `n` = **half-buffer** length in mono `int16_t` samples (typically `sample_rate / fps`). Cap is `AUDIO_BUFFER_LENGTH` (1077) in `gw_audio.h`. |
| `audio_get_active_buffer()` / `audio_get_buffer_length()` | Half currently due for fill. Write mono PCM here.                                                                                                       |
| `common_emu_sound_loop_is_muted()`                        | Skip fill (or write silence) when paused / muted.                                                                                                       |
| `common_emu_sound_get_volume()`                           | Menu volume **0..255**; scale samples yourself (`(s * vol) / 255` or `>> 8`).                                                                           |
| `common_emu_sound_sync(false)`                            | Wait for the next DMA half so you do not overwrite a buffer still playing. Call once per filled half (usually end of frame).                            |
| `odroid_audio_mute(true/false)`                           | Used on pause / `start_paused`.                                                                                                                         |
| `dma_counter` / `common_emu_sound_dma_marker`             | Live ISR / sync counters via ABI pointers (see bridge macros). Needed only for custom pacing.                                                           |


Native wrappers (`audio_*`, `odroid_audio_*`, `common_emu_sound_*`) go through
`audio_ctl` / common helpers in the bridge — do not invent a second path.

### Supported sample rates

Firmware `set_audio_frequency` (via `odroid_audio_init`) accepts these Hz
values. Anything else still stores the request in
`odroid_audio_sample_rate_get()`, but the SAI falls back to **48000**:

8000, 11025, 12000, 16000, 18000, 22050, 31200, 31400, 31440, 32000,
32768, 44100, 48000, 53050, 53267, 63360, 96000, 192000

Half-buffer length must stay ≤ `AUDIO_BUFFER_LENGTH` (1077): e.g.
48000/50 = 960 is fine; 192000 needs a shorter period or will not fit.

### Frame-loop pattern (see `src/main.c`)

1. After system init: `audio_start_playing(sample_rate / fps)`.
2. Each loop: emulate → present → fill `audio_get_active_buffer()` for
  `audio_get_buffer_length()` samples → `common_emu_sound_sync(false)`.
3. If the emulated machine runs slow and skips DMA periods, either catch
  up (fill again + sync) or stretch/resample so every half-buffer has
   fresh PCM — leaving a half unfilled replays stale samples (clicks /
   “stuck” tones).
4. If native rate ≠ DMA rate, mix/resample into the active half (ring
  buffer in RAM_EMU/AHB, then pull). Keep large rings out of DTCM.



### Gotchas

- Buffer is **mono** `int16_t`. Stereo cores must downmix (or pick one
channel) before write.
- `audio_start_playing` while a huge ROM load / memset is running can
starve the CPU; defer start until after heavy init if needed.
- Long APU catch-up without `wdog_refresh()` soft-resets the device.
- Do not place the DMA buffer (or assume AHB `.audio`) in your link image —
firmware owns it. Core mix scratch goes in RAM_EMU / AHB heap.

---



## Extending the ABI

Append-only at the end of `gw_firmware_abi_t` while v2 is in flux (see
header comments). After a public release: bump `GW_FIRMWARE_ABI_VERSION`
only for incompatible layout changes.

In this repo after a firmware change:

```bash
./scripts/sync_from_firmware.sh /path/to/game-and-watch-retro-go-sd
# review diff; fix cores that called removed APIs; rebuild cores
```

`SDK_VERSION` records `FIRMWARE_ABI_VERSION` and `SYNC_DATE`.
