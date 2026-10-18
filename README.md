# Tyrian for Game & Watch Retro-Go SD

<img width="2880" height="2160" alt="image" src="https://github.com/user-attachments/assets/bd51dba2-6a04-4172-8b2a-1ac4ae38c98d" />



Epic MegaGames' **Tyrian** (1995), the vertical shoot'em up, running on the
Nintendo Game & Watch (Mario / Zelda) as a **Retro-Go SD homebrew**
(`/homebrews/Tyrian.bin`, firmware v2.0.0-rc1 or later).

The game engine is **OpenTyrian** (The OpenTyrian Development Team, GPLv2),
from [commercial-game-sources/opentyrian](https://github.com/commercial-game-sources/opentyrian),
ported to the console's 1 MB of RAM and 280 MHz Cortex-M7.

**The game data is not included**, but it is free: Tyrian 2.1 was released
as freeware by its author. Download `tyrian21.zip` (for example from
https://camanis.net/tyrian/tyrian21.zip, the address the OpenTyrian project
gives).

## Installation

See [INSTALL.md](INSTALL.md) (English and French). In short:

1. Unzip `tyrian21.zip`, then build the data pack (Python 3 only):

   ```
   python3 ty_pack.py Tyrian.ty /path/to/tyrian21
   ```

   On Windows: `py ty_pack.py Tyrian.ty C:\Games\tyrian21`

2. Copy both files to the SD card, side by side, with the same name:

   ```
   /homebrews/Tyrian.bin
   /homebrews/Tyrian.ty
   ```

3. Start "Tyrian" from the Homebrew tab. The first start copies the pack to
   the external flash of the console (a minute or two).

### What ty_pack.py does

The AdLib music of Tyrian is synthesized by an OPL emulator while the game
runs: too slow for the console. So `ty_pack.py` runs the game's own music
player on your computer (`ty_render`, shipped for Windows, macOS and Linux in
`ty_render/` next to the script) and stores the songs as samples in the
pack, with their exact loops. Everything else (graphics, levels, sound
effects) is used in place from the flash, as it is in the original files.

| Option | Pack size | Music |
|---|---|---|
| *(default)* | ~26 MB | IMA ADPCM, 11 kHz |
| `--music-codec pcm` | ~73 MB | 16 bit PCM, 11 kHz (no hiss) |
| `--no-music` | ~10 MB | none |

The pack must fit in the external flash of the console; if it does not, the
console says "Pack too big for the flash".

## Controls

| Game & Watch | In a level | In the menus |
|---|---|---|
| D-pad | Fly | Move |
| A | Fire | Validate (Enter) |
| B | Sidekicks (left and right) | Back (Esc) |
| TIME (SELECT on Zelda units) | Change the rear weapon mode | — |
| GAME (START on Zelda units) | Game menu (Esc) | Back (Esc) |
| PAUSE/SET | Retro-Go menu | Retro-Go menu |

The Retro-Go menu adds savestates and, in *Options*: music, sound effects and
"Swap A/B".

**Savestates** work like the game's own saved games: Tyrian saves between
the levels. A savestate records the game as it was when the game menu was
left for the current level (ship, weapons, money, episode, level); loading
it goes back to that game menu. In the game menu itself, it records the
game as it is. The game's own Load / Save (11 slots) works too.

The picture fills the screen (320x200 stretched to 320x240); *Setup* →
*Graphics* → *Picture* → *Original* shows it unstretched, with black bars.

## Building

Needs `arm-none-eabi-gcc` (hard-float, v10+), GNU Make, Python 3 + Pillow:

```
make            # → Tyrian.bin
make docker     # same, inside the Retro-Go SD builder image
```

`ty_render` (the harness, below) is cross-built with zig:

```
pip install ziglang
tools/build_renderers.sh    # → tools/bin/ty_render-<os>-<arch>
```

### Testing without hardware

`harness/` builds the same game and port code for Linux, headless, with a
virtual clock, scripted buttons, PPM screenshots and WAV output:

```
make -C harness
harness/build/ty_host Tyrian.ty -t 30000 -e 1000 -o /tmp/out -w /tmp/out/a.wav
harness/build/ty_host Tyrian.ty -s "12000:A:1,12300:A:0,16000:A:1,16300:A:0" -S 20000 -o /tmp/out
```

`-q file -Q ms` writes a savestate, `-j file -J ms` loads one, `-L file`
restores one at start, `-R dir` renders the music of the pack, `-H kb` sets
the heap (200 is about what the console has). `make -C harness SAN=1` builds
it with AddressSanitizer and UBSan. The pack is mapped read-only, like the
flash.

## Layout

| Path | Role |
|------|------|
| `src/ot/` | OpenTyrian (commercial-game-sources/opentyrian @ 5a9d8da). Game & Watch changes are under `#ifdef GNW`. |
| `src/port/sdl/`, `ty_sdl.c` | The small SDL 2: 8 bit surfaces, keys from the buttons, time, audio callback. |
| `src/port/ty_video.c` | The game's screens, given to the platform. |
| `src/port/ty_loudness.c`, `ty_music.c` | Sound effects in place, pre-rendered music. |
| `src/port/ty_state.c` | Savestates. |
| `src/port/ty_file.c`, `ty_pack.c` | Files of the game: in the pack (flash) or on the SD card. |
| `src/port/ty_main.c` | Retro-Go side: entry point, display (LUT8), audio, buttons, pause menu. |
| `src/port/ty_render.c` | Music rendering (PC only). |
| `tools/ty_pack.py` | Builds the `.ty` pack. |
| `harness/` | Headless test harness, also `ty_render`. |
| `sdk/` | Retro-Go SD core / homebrew SDK (from retro-go-sd-templates). |

See `CLAUDE.md` for the internals.

## License

GPLv2 (see `LICENSE`), like OpenTyrian. The SDK under `sdk/` is MIT
(`sdk/LICENSE.MIT`). Tyrian's data files belong to their authors (freeware).
