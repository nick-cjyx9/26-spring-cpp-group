# AGENTS.md

- DO NOT generate extra `.md` file unless user explicitly asks for it.
- ALWAYS use PowerShell for build-related tasks; assume a Windows environment.

## Repo Structure

The C++ project lives in `campus-rpg/`, **not** at the repo root. All build commands run from inside `campus-rpg/`. The GitHub Actions workflows live at the **repo root** `.github/workflows/` (`auto-build.yml` + `pr-check.yml`; GHA only reads workflows from the repo root).

```text
26-spring-cpp-group/             # repo root — NOT the C++ project root
├── .github/workflows/         # GitHub Actions CI (auto-build.yml + pr-check.yml)
├── .vscode/                   # committed shared VS Code config (see VS Code section)
├── campus-rpg/                # the actual C++ project — run all build commands from here
│   ├── CMakeLists.txt
│   ├── docs/                  # design docs (read campus-rpg/docs/README.md first)
│   ├── src/{core,data,manager,engine/{interfaces,sfml},scenes}/ + main.cpp
│   ├── tests/{test_core.cpp, mocks/}
│   └── resources/             # runtime assets (textures, fonts), copied next to exe on build
├── handouts/  attachments/
├── AGENTS.md  README.md  scripts_cheat_sheet.md
```

## Architecture

Four-layer design:

```text
Engine / Rendering Layer (IRenderer / IWindow / IInput)
  ↓
GameManager (singleton, state coordinator)
  ↓
Core (pure C++ domain models) + Data (SQLite3 persistence)
```

- `src/core/`: `Character`, `Persona`, `Item` + `FoodItem`/`PotionItem`/`SpItem`/`EquipmentItem`/`PersonaItem`, `Inventory`, `Quest`/`QuestManager`, `Enemy` + `Slime`/`Goblin`/`Boss`, `Shop`, `BattleSystem`, `SocialLink`/`SocialLinkManager`, `Entity`, `TileMap`.
- `src/manager/`: `GameManager` singleton holds the single source of truth for game state.
- `src/data/`: `DatabaseManager` + `SaveRepository` handle SQLite3.
- `src/engine/interfaces/`: abstract engine interfaces (`IRenderer`, `IWindow`, `IInput`).
- `src/engine/sfml/`: SFML concrete implementations.
- `src/scenes/`: 2D scene system (`TitleScene`, `TownScene`, `NightScene`, `BattleScene`, `ShopScene`, `InventoryScene`, `CharacterScene`, `DialogueScene`).

### CMake library layout

Sources are compiled into **static libraries** (so the test target links the library instead of re-listing sources); engine interfaces are header-only:

- `CampusRPGCore` (static) — `src/core/*.cpp`, pure-C++ domain models. **No SFML/SQLite dependency.** This is what unit tests link.
- `CampusRPGAppLib` (static) — `src/data/` + `src/manager/` + `src/scenes/*.cpp`. Depends on `CampusRPGCore` + SQLite3. Scenes live here (not in the exe) and must only depend on the engine interfaces, not SFML directly.
- `CampusRPGEngineInterface` (header-only `INTERFACE` lib) — `src/engine/interfaces/` (`IRenderer`, `IWindow`, `IInput`, `IScene`, `EngineTypes.h`).
- `CampusRPG` (executable) — `src/main.cpp` + `src/engine/sfml/*`. Depends on `CampusRPGAppLib` + SFML. Only this target links SFML.
- `CampusRPGTests` (executable) — `tests/test_core.cpp` + `tests/mocks/`. Links `CampusRPGCore` only. Lightweight custom runner (no GUI event loop) — `CHECK`/`CHECK_EQ` macros defined in `test_core.cpp:36-37`.

> - Add a new `src/core/*.cpp` → append to `CORE_SOURCES` in `campus-rpg/CMakeLists.txt`.
> - Add a file under `src/data/`, `src/manager/`, or `src/scenes/` → append to `APP_LIB_SOURCES`.
> - Add an SFML impl under `src/engine/sfml/` → append to `ENGINE_SFML_SOURCES` (exe target).
> - New `src/engine/interfaces/*.h` → no CMake edit needed; the INTERFACE lib auto-picks up headers via include dirs.
> - Design docs live under `campus-rpg/docs/` — read `docs/README.md` first.

OOP checkpoints (course requirement):

- Encapsulation: private fields, public getters/setters.
- Inheritance: abstract `Item`, `Enemy`, `Entity`, `Scene`, engine interfaces `IRenderer`, `IWindow`, `IInput`.
- Polymorphism: `item->use(character)`, `enemy->battleCry()`, `renderer->drawRect(...)`, `scene->update(...)`.
- Association: `GameManager` composes `Character`, `Inventory`, `Shop`, `QuestManager`, `SocialLinkManager`, `TileMap`.

STL checkpoints (course requirement):

- `std::vector<std::unique_ptr<Item>>` for inventory/shop/enemies.
- `std::map<std::string, Quest>` for quest lookup.
- `std::map<std::string, SocialLink>` for social link lookup.
- `std::vector<std::string>` for battle log.

## Build & Test

The project uses **CMake Presets** so teammates do not need identical compiler paths. Pick the preset that matches your toolchain.

```powershell
# All commands run from campus-rpg/
cd campus-rpg

# List available presets
cmake --list-presets

# Configure + build + test (Windows MinGW example)
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release

# Run the app or tests directly
.\build\windows-mingw-release\CampusRPG.exe
.\build\windows-mingw-release\tests\CampusRPGTests.exe
```

Available presets are defined in `campus-rpg/CMakePresets.json`:

| Preset | Toolchain | Notes |
| --- | --- | --- |
| `windows-mingw-release` | MinGW Makefiles / g++ | Default for local development; Qt-bundled or standalone MinGW |
| `windows-msvc-release` | Visual Studio 17 2022 / cl.exe | Run from Developer PowerShell for VS 2022 |
| `linux-gcc-release` | Unix Makefiles / g++ | For WSL / Linux VMs |
| `macos-clang-release` | Unix Makefiles / clang++ | For macOS |

With **multi-config generators** (e.g., Visual Studio) the executable still lands under `build/<preset>/Release/`. With **single-config generators** (e.g., MinGW Makefiles) it lands under `build/<preset>/`.

## Key Gotchas

- **CMake Presets require CMake 3.20+**. Run `cmake --version` first. If your system CMake is older, update it or configure manually with `-G`/`-D` flags.
- **SFML not found**: by default SFML is fetched automatically via CMake `FetchContent`. Ensure network is available during first configure. To use a local SFML, pass `-DCAMPUS_RPG_USE_SYSTEM_SFML=ON -DSFML_DIR=<path/to/SFMLConfig.cmake>`.
- **SQLite3 not found**: similarly fetched automatically. To use a system SQLite3, pass `-DCAMPUS_RPG_USE_SYSTEM_SQLITE3=ON`.
- **MinGW g++ not found**: either add MinGW `bin` to your system `Path`, or configure with explicit compiler paths:
  ```powershell
  cmake --preset windows-mingw-release `
      -DCMAKE_C_COMPILER=E:/Qt/Tools/mingw1310_64/bin/gcc.exe `
      -DCMAKE_CXX_COMPILER=E:/Qt/Tools/mingw1310_64/bin/g++.exe
  ```
- **MSVC missing**: run from **Developer PowerShell for VS 2022** or execute `& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"` before configuring.
- **Test framework**: tests use a lightweight pure-C++ runner (`tests/test_core.cpp`). The core-only test executable links `CampusRPGCore` + mock implementations, so it runs without a GUI event loop in headless/CI environments.
- **CI**: GitHub Actions (`.github/workflows/{auto-build.yml, pr-check.yml}`) build and test on `windows-latest` and `ubuntu-latest`.

## Development Notes

- Keep `core/` free of SFML/SQLite dependencies so unit tests stay fast and portable.
- Scenes call `refresh()` when entered to reflect the latest `GameManager` state.
- `GameManager::newGame()` initializes default shop/quest/enemy/Persona/SocialLink data; load save after this so definitions exist.
- `SaveRepository` stores base stats only; equipment bonuses are recomputed on equip.
- Singletons: `GameManager`, `DatabaseManager`. Do not create multiple instances.
- Privacy: files with personal info (real name, student ID) must use `.secret.xxx` extension — enforced by `.gitignore`.
- Extendable stubs:
  - Equipment currently adds stats permanently on use; add real equipment slots later.
  - Quest conditions only support `kill:N`; add `collect:id:N`, `level:N`, etc.
  - Save/load currently supports one slot; extend for multiple save slots.
  - Persona fusion is a planned optional extension.

## File Ownership & Review (Coordination)

Multiple issues touch the same hot files and risk merge conflicts / overwrites. To let each member focus on their own scope without stepping on each other, the following ownership and review rules apply:

- **`src/core/Character.cpp/.h`** — **Coordination owner: nick-cjyx9**. Any PR changing `Character.cpp/.h` must **open a PR (no direct push to main) and request review from nick-cjyx9**.
- **`src/data/SaveRepository.cpp/.h` + `DatabaseManager.cpp/.h`** — **Owner: W0606**. Persistence layer is centralized here; other members expose read-only getters / propose interface changes via PR rather than editing the data layer directly.
- **`src/manager/GameManager.cpp/.h`** — **Coordination owner: nick-cjyx9**.
- **`src/engine/interfaces/`** — touched by rendering and scene changes; coordinate via PR.
- **`src/scenes/`** — each scene owner should align with `GameManager` state transitions.
- **`src/core/Enemy.*`** — numeric constants only for balance changes.
- **`tests/test_core.cpp`** — curated by tests owner; other members adding tests append under their own clearly-named test functions.

General rule: prefer **small, focused PRs over long-lived feature branches**, and rebase onto `main` before requesting review. Branch `main` is protected — all changes merge via PR with `PR Check` green.

## Testing Best Practices

1. **Verify-first**: add a regression test before fixing a bug.
2. **Descriptive test names**: `shopBuyFailsWithoutEnoughGold()` beats `test1()`.
3. **Self-contained tests**: each test function stands alone.
4. **No abort-on-failure**: use lightweight `CHECK`/`CHECK_EQ` macros instead of `assert`.
5. **Make tests fast**: no fixed sleeps, no I/O on the test path.
6. **Compile classes into a library**: `CampusRPGCore` is a static lib so the test target links it.
7. **Break dependencies**: the pure-C++ core has no SFML/SQLite coupling, so it can be unit-tested in isolation.
8. **Avoid debug spam**: do not leave `std::cout` in committed autotests.
