# AGENTS.md

- DO NOT generate extra `.md` file unless user explicitly asks for it.
- ALWAYS use PowerShell for build-related tasks; assume a Windows environment.
- `gh` cli under powershell environment is provided.

## Repo Structure

The C++ project lives in `campus-rpg/`, **not** at the repo root. All build commands run from inside `campus-rpg/`. The GitHub Actions workflows live at the **repo root** `.github/workflows/` (`auto-build.yml` + `pr-check.yml`; GHA only reads workflows from the repo root).

```text
26-spring-cpp-group/             # repo root
├── .github/workflows/         # GitHub Actions CI (auto-build.yml + pr-check.yml)
├── campus-rpg/                  # the actual C++ project
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── core/              # pure C++ domain models
│   │   ├── data/              # SQLite3 persistence
│   │   ├── manager/           # GameManager singleton
│   │   ├── engine/
│   │   │   ├── interfaces/    # IRenderer / IWindow / IInput
│   │   │   └── sfml/          # SFML implementations
│   │   ├── scenes/            # 2D game scenes
│   │   └── main.cpp
│   ├── tests/
│   │   ├── test_core.cpp
│   │   └── mocks/             # mock engine implementations
│   ├── resources/             # runtime assets (textures, fonts)
│   └── CMakeLists.txt
├── docs/                        # design docs
├── handouts/
├── attachments/
├── AGENTS.md
├── README.md                    # setup guide
└── scripts_cheat_sheet.md
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

Sources are compiled into **static libraries** (so the test target links the library instead of re-listing sources):

- `CampusRPGCore` — pure-C++ domain models (`src/core/`). **No SFML/SQLite dependency.** This is what unit tests link.
- `CampusRPGAppLib` — `src/data/` + `src/manager/` + `src/engine/interfaces/`. Depends on `CampusRPGCore` + SQLite3.
- `CampusRPG` (executable) — `src/engine/sfml/` + `src/scenes/` + `src/main.cpp`. Depends on `CampusRPGAppLib` + SFML.
- `CampusRPGTests` (executable) — `tests/test_core.cpp` + `tests/mocks/`. Depends on `CampusRPGCore` + mock implementations. Uses a lightweight test runner, no GUI event loop required.

> When adding a new `src/core/*.cpp` file, add it to the `CORE_SOURCES` list in `campus-rpg/CMakeLists.txt`. Files under `src/data/` or `src/manager/` go into `APP_LIB_SOURCES`. Engine interface files go into `ENGINE_INTERFACE_SOURCES`. SFML implementation and scene files go into the executable target.
>
> Project design docs live under `campus-rpg/docs/`.

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

Verified working setup: **MinGW Makefiles** generator with **SFML 2.6+** and the g++ shipped under `E:/Qt/Tools/mingw1310_64`.

```powershell
# All commands run from campus-rpg/
cd campus-rpg

# Configure (single-config MinGW Makefiles)
# SFML and SQLite3 are fetched automatically via CMake FetchContent.
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
    -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"

# Build
cmake --build build --config Release --parallel

# Run tests (CTest picks up the CampusRPGTests target)
ctest --test-dir build -C Release --output-on-failure
.\build\tests\CampusRPGTests.exe      # run the runner directly

# Run app
.\build\CampusRPG.exe
```

With the **Visual Studio** generator (multi-config), executables land under `build\Release\` and `build\tests\Release\` instead — adjust the paths accordingly.

### VS Code (shared config)

The repo ships a shared `.vscode/` (IntelliSense, build/test tasks, gdb launch configs) at the repo root. It is **not** gitignored — it is committed so every member gets the same setup. All machine-specific paths go through the `MINGW_BIN` user environment variable or CMake cache.

## Key Gotchas

- **SFML not found**: SFML is fetched automatically via CMake `FetchContent` if not found locally. Ensure network is available during first configure, or set `SFML_DIR` to a local SFML install.
- **MinGW g++ not found**: point `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` at the gcc/g++ bundled with Qt, e.g. `E:/Qt/Tools/mingw1310_64/bin/gcc.exe` and `E:/Qt/Tools/mingw1310_64/bin/g++.exe`.
- **MSVC missing**: run from **Developer PowerShell for VS 2022** or execute `& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`.
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
7. **Break dependencies**: the pure-C++ core has no SFML/SQLite coupling, so it can be unit-tested in isolation. Scene tests use Mock 引擎实现.
8. **Avoid debug spam**: do not leave `std::cout` in committed autotests.
