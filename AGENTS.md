# AGENTS.md

- DO NOT generate extra `.md` file unless user explicitly asks for it.
- ALWAYS use PowerShell for build-related tasks; assume a Windows environment.

## Repo Structure

The C++ project lives in `campus-rpg/`, **not** at the repo root. All build commands run from inside `campus-rpg/`. The GitHub Actions workflow lives at the **repo root** `.github/workflows/ci.yml` (GHA only reads workflows from the repo root).

```text
26-spring-cpp-group/             # repo root
├── .github/workflows/ci.yml     # GitHub Actions CI (build + test)
├── campus-rpg/                  # the actual C++ project
│   ├── CMakeLists.txt
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
├── handouts/
├── attachments/
├── AGENTS.md
├── README.md                    # Chinese setup guide
└── scripts_cheat_sheet.md
```

## Architecture

Four-layer design:

```text
UI (Qt Widgets)
  ↓
GameManager (singleton, state coordinator)
  ↓
Core (pure C++ domain models) + Data (SQLite persistence)
```

- `src/core/`: `Character`, `Item` + `FoodItem`/`PotionItem`/`EquipmentItem`, `Inventory`, `Quest`/`QuestManager`, `Enemy` + `Slime`/`Goblin`/`Boss`, `Shop`, `BattleSystem`.
- `src/manager/`: `GameManager` singleton holds the single source of truth for game state.
- `src/data/`: `DatabaseManager` + `SaveRepository` handle SQLite via QtSql.
- `src/ui/`: Qt Widgets pages. UI only renders and forwards actions; no business logic.

### CMake library layout

Sources are compiled into **static libraries** (so the test target links the library instead of re-listing sources):

- `CampusRPGCore` — pure-C++ domain models (`src/core/`). **No Qt/SQLite dependency.** This is what unit tests link.
- `CampusRPGAppLib` — `src/data/` + `src/manager/`. Depends on `CampusRPGCore` + `Qt6::Core` + `Qt6::Sql`.
- `CampusRPG` (executable) — `src/ui/` + `src/main.cpp`. Depends on `CampusRPGAppLib` + `Qt6::Widgets`.
- `CampusRPGTests` (executable) — `tests/test_core.cpp`. Depends on `CampusRPGCore` only.

> When adding a new `src/core/*.cpp` file, add it to the `CORE_SOURCES` list in `campus-rpg/CMakeLists.txt`. Files under `src/data/` or `src/manager/` go into `APP_LIB_SOURCES`. UI files go into `UI_SOURCES`/`UI_FORMS`.

OOP checkpoints (course requirement):

- Encapsulation: private fields, public getters/setters.
- Inheritance: abstract `Item`, `Enemy` base classes.
- Polymorphism: `item->use(character)`, `enemy->battleCry()`.
- Association: `GameManager` composes `Character`, `Inventory`, `Shop`, `QuestManager`.

STL checkpoints (course requirement):

- `std::vector<std::unique_ptr<Item>>` for inventory/shop/enemies.
- `std::map<std::string, Quest>` for quest lookup.
- `std::vector<std::string>` for battle log.

## Build & Test

Verified working setup: **MinGW Makefiles** generator with **Qt 6.11.1 mingw_64** and the g++ shipped under `E:/Qt/Tools/mingw1310_64`.

```powershell
# All commands run from campus-rpg/
cd campus-rpg

# Configure (single-config MinGW Makefiles)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="E:/Qt/6.11.1/mingw_64" `
    -G "MinGW Makefiles" `
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

The repo ships a shared `.vscode/` (IntelliSense, build/test tasks, gdb launch configs) at the repo root. It is **not** gitignored — it is committed so every member gets the same setup. All machine-specific paths go through two **user environment variables** that each member must set (see README §2.3):

- `QT_DIR` — Qt install root, e.g. `E:/Qt/6.11.1/mingw_64`
- `QT_MINGW_BIN` — MinGW bin dir bundled with Qt, e.g. `E:/Qt/Tools/mingw1310_64/bin`

The C/C++ extension uses explicit `includePath` entries (not `compile_commands.json`) because CMake/MinGW puts Qt include flags in a response file (`@includes_CXX.rsp`) that IntelliSense does not expand — that is what made `QApplication` show as unresolvable.

## Key Gotchas

- **Qt not found**: pass `CMAKE_PREFIX_PATH` explicitly, e.g. `E:/Qt/6.11.1/mingw_64` (MinGW) or `C:/Qt/6.8.0/msvc2022_64` (MSVC).
- **MinGW g++ not found**: point `CMAKE_CXX_COMPILER` at the g++ bundled with Qt, e.g. `E:/Qt/Tools/mingw1310_64/bin/g++.exe`. Mixing the system MinGW (e.g. `D:/mingw64`) with Qt's MinGW build can cause ABI mismatches.
- **MSVC missing**: run from **Developer PowerShell for VS 2022** or execute `& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"`.
- **Test framework**: tests use a **pure-C++ lightweight runner** (`tests/test_core.cpp`) — no Qt, no event loop, no external test lib. The runner records failures without aborting (mirroring `QCOMPARE`/`QVERIFY` semantics), prints a `run: N failed: M` summary, and returns a non-zero exit code on failure so CTest picks it up. Qt Test was avoided because its `qExec` blocks in headless/non-interactive Windows sessions; the pure-C++ runner runs everywhere.
- **CI**: GitHub Actions (`.github/workflows/ci.yml`) builds and tests on `windows-latest` and `ubuntu-latest` using `jurplel/install-qt-action`. Pushes trigger on `main`/`dev`; PRs target `main`.

## Development Notes

- Keep `core/` free of Qt/SQLite dependencies so unit tests stay fast and portable.
- UI pages call `refresh()` before `show()` to reflect the latest `GameManager` state.
- `GameManager::newGame()` initializes default shop/quest/enemy data; load save after this so quest definitions exist.
- `SaveRepository` stores base stats only; equipment bonuses are recomputed on equip.
- Singletons: `GameManager`, `DatabaseManager`. Do not create multiple instances.
- Privacy: files with personal info (real name, student ID) must use `.secret.xxx` extension — enforced by `.gitignore`.
- Extendable stubs:
  - Equipment currently adds stats permanently on use; add real equipment slots later.
  - Quest conditions only support `kill:N`; add `collect:id:N`, `level:N`, etc.
  - Save/load currently supports one slot; extend for multiple save slots.

## Testing Best Practices (simplified)

Ref (Qt Test docs, principles apply to our pure-C++ runner too):

- <https://doc.qt.io/qt-6/zh/qttest-index.html> — Qt Test module overview
- <https://doc.qt.io/qt-6/zh/qtest-overview.html> — creating & running tests
- <https://doc.qt.io/qt-6/zh/qttest-best-practices.html> — best-practices guide
- <https://doc.qt.io/qt-6/zh/best-practices.html> — general Qt best-practices index

Distilled rules we follow:

- **Verify-first**: add a regression test before fixing a bug; the test must fail before the fix and pass after.
- **Descriptive names**: `testShopBuyFailsWithoutEnoughGold` beats `test1`. The name is the failure message.
- **Self-contained tests**: each test function stands alone — no dependence on a previous test having run. Instantiate the object under test on the stack so it cleans up even on failure.
- **No abort-on-failure**: record the failure and keep going (like `QCOMPARE`), never `Q_ASSERT`/`assert` — aborting skips cleanup and the rest of the suite.
- **Make tests fast**: no fixed sleeps, no I/O on the test path. Keep benchmarks out of `tests/`.
- **No side effects in checks**: a `CHECK`/`QCOMPARE` expression must not mutate state; it may be evaluated more than once.
- **Compile classes into a library**: `CampusRPGCore` is a static lib so the test target links it instead of re-listing sources.
- **Break dependencies**: the pure-C++ core has no Qt/SQLite coupling, so it can be unit-tested in isolation.
