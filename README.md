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

## 二、Windows 开发环境配置（PowerShell）

以下命令全部在 **PowerShell** 中执行。

### 1. 安装基础工具

#### 1.1 安装 Git

```powershell
winget install --id Git.Git -e --source winget
git --version
```

#### 1.2 安装 CMake

```powershell
winget install --id Kitware.CMake -e --source winget
cmake --version
```

#### 1.3 安装 MinGW（Qt 自带的 MinGW 即可）

如果使用 Qt 安装目录下的 MinGW：

```powershell
E:\Qt\Tools\mingw1310_64\bin\g++.exe --version
```

或安装独立 MinGW-w64。

### 2. 安装 SFML

#### 方式 A：手动下载

1. 从 [SFML 官网](https://www.sfml-dev.org/download.php) 下载 Windows 64-bit MinGW 版本。
2. 解压到 `E:\SFML` 或 `C:\SFML`。

#### 方式 B：使用 CMake FetchContent（推荐）

本项目 `CMakeLists.txt` 已配置 `FetchContent` 自动下载 SFML，无需手动安装。

### 3. 安装 SQLite3

SQLite3  amalgamation 单文件版或预编译库均可。本项目使用 `FetchContent` 自动获取 SQLite3。

### 4. 配置环境变量

如果使用独立 MinGW，请将其 `bin` 加入用户 `Path`：

```powershell
$mingwBin = "E:\Qt\Tools\mingw1310_64\bin"
$oldPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($oldPath -notlike "*$mingwBin*") {
    [Environment]::SetEnvironmentVariable("Path", "$oldPath;$mingwBin", "User")
}
```

### 5. 克隆仓库并构建

```powershell
Set-Location E:\dev\my-schoolworks\26-spring-cpp-group

git clone https://github.com/nick-cjyx9/26-spring-cpp-group.git
cd campus-rpg

# 创建构建目录并配置（MinGW Makefiles）
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -G "MinGW Makefiles" `
    -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"

# 编译
cmake --build build --config Release --parallel
```

### 6. 运行程序

```powershell
.\build\CampusRPG.exe
```

### 7. 运行单元测试

```powershell
# 方法 1：通过 ctest
ctest --test-dir build -C Release --output-on-failure

# 方法 2：直接运行测试可执行文件
.\build\tests\CampusRPGTests.exe
```

测试只链接纯 C++ 核心库 `CampusRPGCore` 和 Mock 实现，不依赖 SFML/SQLite，可在命令行直接运行。

## 三、IDE 配置

### VS Code

仓库已自带共享的 `.vscode/` 配置（IntelliSense、构建/测试任务、调试）。首次打开会提示安装推荐扩展（C/C++、CMake Tools）。

**环境变量**：设置用户环境变量 `MINGW_BIN` 为你的 MinGW `bin` 目录（例如 `E:/Qt/Tools/mingw1310_64/bin`），然后重启 VS Code。`.vscode/` 配置会读取该变量来定位编译器和调试器。

### CLion

直接打开 `campus-rpg/CMakeLists.txt` 即可。

## 四、项目结构

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
│   └── CMakeLists.txt         # CMake 主配置
├── docs/                       # 设计文档
├── handouts/                   # 检查点材料
├── AGENTS.md
├── README.md                  # 本文件
└── scripts_cheat_sheet.md
```

## 五、Git 协作规范

```powershell
git checkout -b feature/xxx

# 提交前先在本地构建测试通过
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

git add .
git commit -m "feat: xxx"
git push -u origin feature/xxx
```

## 六、完成的挑战任务

| 挑战 | 内容 |
| --- | --- |
| 挑战一：数据库技术 | SQLite3 持久化 |
| 挑战三：图形用户界面技术 | SFML 2D 图形界面 |
| 挑战六：STL 高级应用 | `map`、`vector`、`algorithm` 综合使用 |
| 挑战八：软件工程实践 | Git 协同、单元测试、自动化构建、持续集成 |
