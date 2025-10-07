# Freeze Tag Quake 2 Mod - Agent Guide

**See [.github/copilot-instructions.md](.github/copilot-instructions.md) for comprehensive project documentation.**

## Build Commands
- **Build**: Open `game.sln` in Visual Studio 2022 or run `msbuild game.sln /p:Configuration=Release /p:Platform=x64`
- **Deploy & Test**: Run `freezetag.bat` to copy DLL to mod directory and launch Quake 2 with bots
- **No unit tests**: This is a game mod - test by running the game

## Quick Architecture
- **Type**: C++ DLL mod for Quake 2 Rerelease - builds `game_x64.dll`
- **Core Files**: `src/g_*.cpp` (game rules), `src/p_*.cpp` (player/movement), `src/m_*.cpp` (monsters)
- **Freeze Tag Logic**: `src/g_freeze.cpp` and `src/g_freeze.h`
- **Movement/Prediction**: `src/p_move.cpp` - see `PMF_NO_POSITIONAL_PREDICTION` flags in `game.h`

## Code Style
- **Language**: C++17/20, uses Quake-style APIs (`edict_t`, `gclient_t`, `cvar_t`)
- **Headers**: Include `g_local.h` first, then specific headers
- **Naming**: Lowercase with underscores (`frozen_time`, `g_freeze.cpp`), macros uppercase
- **Comments**: Mark freeze-tag additions with `/* freeze */` comment blocks
- **Formatting**: Tab indentation, opening braces on same line
- **Safety**: Don't modify ABI/struct layouts in `game.h` or exported symbols without understanding engine compatibility
