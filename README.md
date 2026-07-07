# Campus RPG Adventure Game System

基于 C++20 + SFML + SQLite3 的 2D 校园 RPG 冒险游戏系统，用于 2026 年春季学期《程序设计课程设计》大作业。
1

## 项目特色

- **2D 顶视角 RPG**：玩家可在城镇/学校地图自由移动，白天进行 Social Link，夜晚与阴影战斗。
- **Persona 机制**：主角装备人格面具获得属性、抗性与技能，战斗中可切换。
- **简化回合制战斗**：保留元素弱点/耐性，但不引入复杂的 1 More / 总攻击。
- **完整基础系统**：角色、背包、商店、任务、等级成长、存档读档。
- **抽象引擎接口**：`IRenderer`/`IWindow`/`IInput` 解耦上层与 SFML，支持 Mock 测试。

## 须知

### 个人隐私信息保护

任何包含真实姓名、学号、联系方式等个人信息的文件，请保存为 `.secret.xxx` 格式（如 `小组成员及分工.secret.md`）保证不被上传 GitHub，大家不会被开盒。

## 一、技术栈

| 层级 | 技术 |
| ------ | ------ |
| 编程语言 | C++20 |
| 2D 引擎 | SFML 2.6+ |
| 数据库 | SQLite3 |
| 构建系统 | CMake 3.20+ |
| 单元测试 | 纯 C++ 轻量测试框架（核心层无图形/数据库依赖） |
| 版本控制/CI | Git + GitHub + GitHub Actions |

## 二、快速开始（CMake Presets，推荐）

项目使用 **CMake Presets** 管理不同平台/工具链的配置，避免在文档或配置文件中硬编码编译器路径。

```powershell
# 进入项目目录
cd campus-rpg

# 查看可用 presets
cmake --list-presets

# 选择一个 preset 进行配置+构建+测试（以 Windows MinGW 为例）
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release

# 运行程序
.\build\windows-mingw-release\CampusRPG.exe
```

> 多配置生成器（如 Visual Studio）的可执行文件仍可能在 `build/<preset>/Release/` 下，具体以 `cmake --build --preset <name>` 输出为准。

## 三、环境配置

### 3.1 安装基础工具

#### Git

```powershell
winget install --id Git.Git -e --source winget
git --version
```

#### CMake 3.20+

```powershell
winget install --id Kitware.CMake -e --source winget
cmake --version
```

### 3.2 安装 C++ 编译器（任选其一）

本仓库支持 **MinGW**、**MSVC**、**Linux GCC**、**macOS Clang**。你不需要和队友装完全一样的工具链。

#### 选项 A：MinGW（Qt 自带或独立安装）

如果你装了 Qt，通常已经自带 MinGW：

```powershell
# 示例路径，请换成你机器上的实际路径
E:\Qt\Tools\mingw1310_64\bin\g++.exe --version
```

建议把 MinGW 的 `bin` 目录加入用户 `Path`，方便 VS Code / 终端调用 `gcc`/`g++`/`gdb`：

```powershell
$mingwBin = "E:\Qt\Tools\mingw1310_64\bin"
$oldPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($oldPath -notlike "*$mingwBin*") {
    [Environment]::SetEnvironmentVariable("Path", "$oldPath;$mingwBin", "User")
}
```

#### 选项 B：MSVC（Visual Studio 2022）

安装 **Desktop development with C++** 工作负载。从 **Developer PowerShell for VS 2022** 打开终端，或者手动运行：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

#### 选项 C：Linux / macOS

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y build-essential cmake git

# macOS
xcode-select --install
brew install cmake
```

### 3.3 依赖说明

SFML 和 SQLite3 默认通过 CMake `FetchContent` 自动下载，**无需手动安装**。

如果你希望使用系统已安装的库（高级用户/加速 CI）：

```powershell
cmake --preset windows-mingw-release -DCAMPUS_RPG_USE_SYSTEM_SFML=ON -DCAMPUS_RPG_USE_SYSTEM_SQLITE3=ON
```

## 四、命令行构建

所有命令都在 `campus-rpg/` 目录下执行。

```powershell
cd campus-rpg

# 1. 选择并应用 preset
cmake --preset windows-mingw-release

# 2. 编译
cmake --build --preset windows-mingw-release

# 3. 运行测试
ctest --preset windows-mingw-release

# 4. 运行程序
.\build\windows-mingw-release\CampusRPG.exe
```

如果你不用 presets，也可以手动指定生成器和编译器：

```powershell
# MinGW 手动示例（路径请按实际情况修改）
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

> 不推荐新手手动指定，因为不同人生成器/路径不同，容易出错。

## 五、VS Code 配置

仓库已自带共享的 `.vscode/` 配置（IntelliSense、构建/测试任务、调试）。首次打开会提示安装推荐扩展（C/C++、CMake Tools）。

### 5.1 使用 CMake Presets

1. 打开命令面板（`Ctrl+Shift+P`）。
2. 选择 **CMake: Select Configure Preset**。
3. 根据你的工具链选择：
   - `windows-mingw-release`（Qt/独立 MinGW）
   - `windows-msvc-release`（Visual Studio 2022）
   - `linux-gcc-release`
   - `macos-clang-release`
4. VS Code 会自动完成配置、构建目录设置和 IntelliSense。

### 5.2 常用任务

| 任务 | 操作 |
| --- | --- |
| 构建 | `Ctrl+Shift+B` 或 **CMake: Build** |
| 运行测试 | **CMake: Run Tests** |
| 调试 | `F5` → 选择 `Debug CampusRPG (GDB / MinGW)` 或 `Debug CampusRPG (VS Debugger / MSVC)` |
| 运行程序 | **CMake: Debug -> Run without Debugging** 或任务 `run-app` |

### 5.3 调试器路径

- **MinGW 用户**：建议把 MinGW `bin` 加入系统 `Path`，或设置环境变量 `MINGW_BIN`（如 `E:/Qt/Tools/mingw1310_64/bin`）供 `.vscode/launch.json` 使用。
- **MSVC 用户**：使用 `Debug CampusRPG (VS Debugger / MSVC)` 配置，无需 `MINGW_BIN`。

### 5.4 CLion / 其他 IDE

直接打开 `campus-rpg/CMakeLists.txt`，CMakePresets.json 会被自动识别。

## 六、常见问题

### 6.1 `cmake --preset` 提示找不到 preset

请确认：

- CMake 版本 ≥ 3.20（`cmake --version`）。
- 当前目录是 `campus-rpg/`（`CMakePresets.json` 所在目录）。
- 运行 `cmake --list-presets` 查看可用 preset。

### 6.2 SFML / SQLite3 下载失败

首次配置需要联网下载依赖。如果网络不畅：

1. 科学上网后重试；
2. 或手动下载 SFML 2.6+ 和 SQLite3，使用系统库选项：

```powershell
cmake --preset windows-mingw-release -DCAMPUS_RPG_USE_SYSTEM_SFML=ON -DSFML_DIR="E:/SFML/lib/cmake/SFML"
```

### 6.3 MinGW 找不到 `gcc`/`g++`

- 把 MinGW `bin` 加入系统 `Path`；或
- 在 CMake 配置时显式指定编译器：

```powershell
cmake --preset windows-mingw-release -DCMAKE_C_COMPILER=E:/Qt/Tools/mingw1310_64/bin/gcc.exe -DCMAKE_CXX_COMPILER=E:/Qt/Tools/mingw1310_64/bin/g++.exe
```

### 6.4 MSVC 提示找不到编译器

确保从 **Developer PowerShell for VS 2022** 执行命令，或已运行 `vcvars64.bat`。

### 6.5 运行时提示缺少 DLL

MinGW 构建时，CMake 会自动把 SFML DLL 复制到可执行文件旁边。如果仍缺 DLL，请把 MinGW `bin` 目录加入 `Path` 后重新运行。

## 七、项目结构

开始开发前，请先人工阅读一遍 `campus-rpg\docs` 下的东西，至少得读一遍 `campus-rpg\docs\README.md`, 对你的 AI 要做的东西有一个最基本的了解再派活给 AI。

```text
26-spring-cpp-group/
├── .github/workflows/          # GitHub Actions
├── .vscode/                   # 共享 VS Code 配置
├── campus-rpg/
│   ├── src/
│   │   ├── core/              # 核心领域模型（纯 C++）
│   │   ├── data/              # SQLite3 数据库管理
│   │   ├── manager/           # GameManager 总控
│   │   ├── engine/
│   │   │   ├── interfaces/    # IRenderer/IWindow/IInput
│   │   │   └── sfml/          # SFML 实现
│   │   ├── scenes/            # 2D 场景（城镇/夜晚/战斗/商店等）
│   │   └── main.cpp           # 程序入口
│   ├── tests/
│   │   ├── test_core.cpp      # 核心测试
│   │   └── mocks/             # Mock 引擎实现
│   ├── resources/             # 图标、资源文件
│   ├── CMakeLists.txt         # CMake 主配置
│   └── CMakePresets.json      # 跨平台构建预设
├── docs/                       # 设计文档
├── handouts/                   # 检查点材料
├── AGENTS.md
├── README.md                  # 本文件
└── scripts_cheat_sheet.md
```

## 八、Git 协作规范

```powershell
git checkout -b feature/xxx

# 提交前先在本地构建测试通过
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release

git add .
git commit -m "feat: xxx"
git push -u origin feature/xxx
```

## 九、完成的挑战任务

| 挑战 | 内容 |
| --- | --- |
| 挑战一：数据库技术 | SQLite3 持久化 |
| 挑战三：图形用户界面技术 | SFML 2D 图形界面 |
| 挑战六：STL 高级应用 | `map`、`vector`、`algorithm` 综合使用 |
| 挑战八：软件工程实践 | Git 协同、单元测试、自动化构建、持续集成 |
