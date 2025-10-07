## Freeze Tag (Quake II rerelease) — Copilot instructions

Short, focused guidance to help an AI coding agent be productive in this repository.

- Project type: native C++ game mod that builds a Windows x64 DLL (`game_x64.dll`) to be dropped into the Quake II rerelease `rerelease\freezetag` folder. The main code lives in `src/` and follows Quake-2 game module conventions.

- Key files and folders:
  - `src/` — implementation. Look at `game.h` for the public game API and constants (GAME_API_VERSION, CGAME_API_VERSION, MAX_CLIENTS, etc.).
  - `src/g_*.cpp`, `src/p_*.cpp`, `src/m_*.cpp` — game logic, player movement, and monsters. `g_freeze.cpp` and `g_freeze.h` contain the mod-specific freeze-tag rules.
  - `game.vcxproj` and `game.sln` — Visual Studio project/solution used to build the DLL on Windows.
  - `src/vcpkg.json` — vcpkg-managed dependencies (fmt, jsoncpp). Use vcpkg to install these before building.
  - `freezetag.bat` — convenience script showing the install + run steps: copies `game_x64.dll`, `autoexec.cfg`, `freeze.cfg` into the local Quake2 rerelease folder and launches `quake2.exe` with recommended launch args.
  - `freeze.cfg`, `autoexec.cfg` — runtime config and recommended console settings for the mod.

- Big picture architecture and conventions:
  - This is a drop-in game module for the Quake II engine. The engine loads `game_x64.dll` and calls exported game API functions declared in `game.h`.
  - Many files implement engine callbacks and game rules: `g_*` files are server-side game rules and game state; `p_*` files are player/prediction code; `m_*` files are monster AI. Modify behavior in the appropriate file (e.g., freezing logic lives in `g_freeze.cpp`).
  - The codebase follows Quake-style C APIs (edict_t, gclient_t, cvar_t). Watch for bitflag enums and the MAKE_ENUM_BITFLAGS macro in `game.h`.

- Build / developer workflow (how to build & run):
  - Recommended on Windows with Visual Studio 2019/2022 (see `game.sln`/`game.vcxproj`).
  - Ensure vcpkg dependencies are available: run vcpkg install for `fmt` and `jsoncpp` (vcpkg manifest is `src/vcpkg.json`). If using vcpkg integration with Visual Studio, open the solution and build.
  - After building, copy `game_x64.dll` to your local Quake 2 rerelease folder (example path shown in `freezetag.bat`) and run Quake2 with `+set game freezetag +exec freeze.cfg`.
  - `freezetag.bat` is a working example of the copy + launch steps; prefer using it when testing locally.

- Patterns and project-specific conventions:
  - API/ABI versioning matters: `GAME_API_VERSION` and `CGAME_API_VERSION` in `game.h` must match engine expectations. Don't change them unless you understand engine compatibility.
  - Movement/prediction code is sensitive to binary layout: `pmove_state_t` and related structs must remain bit-compatible with the engine. Avoid changing public structs without updating engine-side code.
  - `g_freeze.cpp` implements game rules for freezing/thawing — search there for mod-specific logic before adding new features.
  - Bots live under `src/bots/` and may not fully support all mod features (note in README: bots don't unfreeze teammates). Keep bot behavior in mind when adding gameplay changes.

- Integration points & external dependencies:
  - The engine (external, not in this repo) loads the DLL and calls exported functions. Treat exported functions and data layouts as stable integration boundaries.
  - Uses vcpkg-managed third-party libs: `fmt` and `jsoncpp` (see `src/vcpkg.json`). Linking and include paths are controlled by the Visual Studio project.

- Useful code pointers / examples:
  - To find where players are frozen/unfrozen: search `src/g_freeze.cpp` and `src/g_freeze.h`.
  - To add a gameplay hook or console command, follow existing patterns in `g_svcmds.cpp` and `g_cmds.cpp`.
  - For player movement and prediction changes (e.g., grapple hook), inspect `p_move.cpp` and `pmflags_t` in `game.h` (see `PMF_NO_POSITIONAL_PREDICTION`).

- Safety and edit rules for AI assistance:
  - Avoid changing ABI/struct layouts (`game.h`) or exported symbols unless implementing an intentional API bump. If you must, note the required engine changes.
  - Prefer small, localized edits: change `g_freeze.cpp` to adjust mod behavior; change `p_move.cpp` for movement; change `m_*.cpp` for monsters.
  - Preserve coding style and macros (e.g., MAKE_ENUM_BITFLAGS). Follow existing naming conventions and file separation.

- If you need to run tests or reproduce problems locally:
  - Use `freezetag.bat` as the minimal reproduction flow: build, copy `game_x64.dll`, then run the bat to launch Quake2 with bots.

If anything in these notes is unclear or you'd like me to include more examples (function names, typical code edits, or build commands), tell me what area to expand and I'll iterate.
