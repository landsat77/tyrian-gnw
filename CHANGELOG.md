# Changelog

This file follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). Release tags must
match a section heading exactly (for example `v1.0.0`): CI uses the matching
section as the GitHub Release notes.

## [Unreleased]

## [v1.0.2] - 2026-10-18

### Fixed

- Crash (Hardfault) when a demo starts, from the title menu (Demo) or after
  waiting on the title screen: the level loader had 75 KB of local
  variables, more than the console's 24 KB stack, and ran over the weapon
  table. Its picture buffer is now the game's third screen (free between
  the levels) and its map buffer is static.
- The statistics of the console log give the free stack.

## [v1.0.1] - 2026-10-18

### Fixed

- Crash (Hardfault) on the console when moving in the title menu: the keys
  without a character no longer give the game SDL's large key codes, which
  the title screen passes to `toupper()` (out of the C library's table on
  the console).

## [v1.0.0] - 2026-10-18

### Added

- Tyrian 2.1 (the freeware PC version) as a Retro-Go SD GWHB homebrew, built
  on OpenTyrian and the retro-go-sd-templates SDK (firmware ABI v2,
  Retro-Go SD v2.0.0-rc1 or later), on a small SDL 2 written for the console.
- Game data from a `.ty` pack next to the `.bin`, used in place from the
  external flash (`tools/ty_pack.py`), with the AdLib music pre-rendered on
  the PC with its exact loops (`ty_render` for Windows, macOS and Linux).
- Savestates like the game's saved games (back to the game menu before the
  level), resume from the launcher.
- Pause menu options: music, sound effects, swap A/B.
- Full screen picture (320x200 stretched to 320x240) or the original size
  (Setup → Graphics → Picture).
