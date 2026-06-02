# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

FreedroidClassic is a C remake of the Commodore 64 game Paradroid. Players control a robot (the "Influence") that must take over enemy robots on a spaceship. It uses SDL3 for graphics, input, and audio.

## Build

```sh
# First time (from git checkout)
./00boot          # bootstraps autotools
./configure       # detects SDL3, SDL3_image, SDL3_mixer, libpng, libjpeg, zlib, libvorbis
make

# Run without installing
./src/freedroid

# Strict build (treats warnings as errors — used by CI)
make strict

# Debug build (AddressSanitizer + UBSan, -Og)
make freedroidDebug
./src/freedroidDebug
```

The `configure` script accepts `--with-sdl-prefix=DIR` if SDL3 is installed in a non-standard location.

There are no automated tests. CI (`.github/workflows/c-cpp.yml`) only verifies that the project compiles cleanly with both GCC and Clang using `-Werror`.

## Architecture

All source is in `src/`. Global state is split across several header-only files that are `#include`d throughout:

- `defs.h` — constants, macros, flag bits
- `struct.h` — all struct definitions (Droids, Maps, Bullets, etc.)
- `global.h` / `vars.h` — global variable declarations
- `proto.h` — all function prototypes

The main game loop lives in `main.c` → `main()`. The startup sequence is:

```
main() → InitFreedroid() [init.c] → InitNewMission() [init.c]
  └─ frame loop:
       ReactToSpecialKeys()   [input.c]
       MoveInfluence()        [influ.c]
       MoveEnemys()           [enemy.c]
       Assemble_Combat_Picture() [graphics.c]
       FD_UpdateWindowSurface()  [sdl_compat.c]
```

### Major subsystems

| File | Responsibility |
|---|---|
| `graphics.c` | SDL3 window/surface management, image loading, fullscreen handling, all rendering |
| `sdl_compat.c/h` | Shim layer bridging SDL2-style API to SDL3; `FD_SetVideoMode()` replaces `SDL_SetVideoMode()` |
| `map.c` | Level file loading, waypoints, doors, lift tiles |
| `ship.c` | Multi-level spaceship structure; manages transitions between floors |
| `menu.c` | All menus — main menu, in-game pause, options |
| `takeover.c` | Takeover minigame (the core unique mechanic) |
| `influ.c` | Player movement, collision, energy |
| `enemy.c` | Enemy AI, pathfinding, combat |
| `input.c` | SDL3 event polling, keybindings |
| `sound.c` | SDL3_mixer audio: sound effects and music |
| `BFont.c` / `text.c` | Bitmap font rendering |

### SDL3 port status

The codebase is mid-port from SDL2 → SDL3 (branch `feature/web-port-sdl3`). Key differences handled in `sdl_compat.h`:

- `SDL_UpdateWindowSurface` / `SDL_UpdateWindowSurfaceRects` are redefined to `FD_UpdateWindowSurface` / `FD_UpdateWindowSurfaceRects` which handle SDL3's pending-size state machine.
- Deprecated `SDLK_KP0`–`SDLK_KP9` are aliased to `SDLK_KP_0`–`SDLK_KP_9`.
- `SDL_FULLSCREEN`, `SDL_SRCCOLORKEY`, `SDL_SRCALPHA` flag constants are defined locally since SDL3 removed them.
- `SDLMod` is typedef'd to `SDL_Keymod`.

When touching rendering or window code, always go through `sdl_compat.h`'s wrappers rather than calling SDL3 surface-update functions directly.

### Themes and data files

Graphics are in `graphics/<theme_name>/` (e.g. `classic_theme/`, `lanzz_theme/`). Each theme has a `config.theme` file. Maps are in `map/`. Sounds are in `sound/`. The game locates these via the `LOCAL_DATADIR` compile-time define (set to the repo root by the build system).
