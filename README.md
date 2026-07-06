# Campus RPG Adventure Game System

基于 C++20 + Qt6 + SQLite 的校园 RPG 冒险游戏系统，用于 2026 年春季学期《程序设计课程设计》大作业。

## 须知

### 个人隐私信息保护

任何包含真实姓名、学号、联系方式等个人信息的文件，请保存为 .secret.xxx 格式（如 小组成员及分工.secret.md）保证不被上传 GitHub，大家不会被开盒。

## 一、技术栈

| 层级 | 技术 |
| ------ | ------ |
| 编程语言 | C++20 |
| GUI 框架 | Qt 6 (Widgets) |
| 数据库 | SQLite（通过 QtSql 访问） |
| 构建系统 | CMake 3.20+ |
| 单元测试 | 纯 C++ 轻量测试运行器（无外部依赖，随 CTest 运行） |
| 版本控制/CI | Git + GitHub + GitHub Actions |

## 二、Windows 开发环境配置（PowerShell）

以下命令全部在 **PowerShell** 中执行，请确保以管理员身份打开 PowerShell。

### 1. 安装基础工具

#### 1.1 安装 Git

```powershell
# 使用 winget 安装 Git
winget install --id Git.Git -e --source winget

# 安装完成后重新打开 PowerShell，验证
git --version
```

#### 1.2 安装 CMake

```powershell
# 使用 winget 安装 CMake
winget install --id Kitware.CMake -e --source winget

# 验证
cmake --version
```

#### 1.3 安装 Visual Studio 2022 社区版（或 Build Tools）

需要 **“使用 C++ 的桌面开发”** 工作负载。

```powershell
# 下载 Visual Studio 安装器
$vsUrl = "https://aka.ms/vs/17/release/vs_community.exe"
$vsInstaller = "$env:TEMP\vs_community.exe"
Invoke-WebRequest -Uri $vsUrl -OutFile $vsInstaller

# 安装社区版 + C++ 桌面开发工作负载
& $vsInstaller --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended --passive --norestart
```

如果你只需要命令行编译工具，可以安装 **Build Tools**：

```powershell
$vsUrl = "https://aka.ms/vs/17/release/vs_buildtools.exe"
$vsInstaller = "$env:TEMP\vs_buildtools.exe"
Invoke-WebRequest -Uri $vsUrl -OutFile $vsInstaller
& $vsInstaller --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive --norestart
```

> 安装完成后，建议重启 PowerShell。

### 2. 安装 Qt 6

Qt 官方目前推荐的安装方式是 **Qt Online Installer**。在国内建议配合中科大/清华镜像下载，速度更快，且安装器会跟随系统语言显示中文界面。

#### 方式 A：官方在线安装器 + 中科大镜像（推荐）

1. 下载在线安装器（中科大镜像）：

```powershell
$qtUrl = "https://mirrors.ustc.edu.cn/qtproject/official_releases/online_installers/qt-online-installer-windows-x64-online.exe"
$qtInstaller = "$env:TEMP\qt-online-installer-windows-x64-online.exe"
Invoke-WebRequest -Uri $qtUrl -OutFile $qtInstaller
```

2. 启动安装器并指定国内镜像源：

```powershell
& $qtInstaller --mirror https://mirrors.ustc.edu.cn/qtproject
```

> 如果安装器界面不是中文，请检查 Windows 系统语言是否设置为中文（设置 → 时间和语言 → 语言和区域 → Windows 显示语言）。

3. 安装时选择：
   - **Qt 6.5 LTS**（或更高版本，如 6.8 LTS）
   - 在 `Qt 6.x.x` 下勾选：
     - `MSVC 2019 64-bit` 或 `MSVC 2022 64-bit`
     - 如需使用 Qt Creator 内置 MinGW，也可勾选 `MinGW`
     - `Qt 5 Compatibility Module`（可选）
   - **Developer and Designer Tools** 中勾选 **Qt Creator**

#### 方式 B：使用 aqtinstall 命令行安装（轻量，无 Qt Creator）

适合不需要 Qt Creator、只想要命令行快速部署的同学。

```powershell
# 安装 aqtinstall
pip install aqtinstall

# 安装 Qt 6.5.3 + MSVC2019 64-bit + 必要模块（Widgets、Sql、Test）
aqt install-qt windows desktop 6.5.3 win64_msvc2019_64 --modules qtbase --outputdir C:\Qt

# 或者安装 MinGW 版本
# aqt install-qt windows desktop 6.5.3 win64_mingw --modules qtbase --outputdir C:\Qt
```

### 3. 配置环境变量

将 Qt 安装路径加入系统环境变量，方便 CMake 找到 Qt。

本文档示例以 MinGW 版 Qt 6.11.1（安装路径 `E:\Qt\6.11.1\mingw_64`，编译器 `E:\Qt\Tools\mingw1310_64\bin\g++.exe`）为准。若你安装的是 MSVC 版，请把路径换成对应的 `msvc2022_64` 目录并改用 Visual Studio 生成器。

```powershell
# 设置 Qt 根目录与 MinGW 工具链 bin（根据实际安装路径修改）
$qtRoot    = "E:\Qt\6.11.1\mingw_64"
$mingwBin  = "E:\Qt\Tools\mingw1310_64\bin"

# 添加到用户环境变量（团队共享的 .vscode 配置会读取这两个变量）
[Environment]::SetEnvironmentVariable("QT_DIR", $qtRoot, "User")
[Environment]::SetEnvironmentVariable("QT_MINGW_BIN", $mingwBin, "User")
[Environment]::SetEnvironmentVariable("Qt6_DIR", "$qtRoot\lib\cmake\Qt6", "User")

# 将 Qt 的 bin 目录加入 PATH
$oldPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($oldPath -notlike "*$qtRoot\bin*") {
    [Environment]::SetEnvironmentVariable("Path", "$oldPath;$qtRoot\bin", "User")
}

# 刷新当前 PowerShell 会话的环境变量
$env:QT_DIR = $qtRoot
$env:QT_MINGW_BIN = $mingwBin
$env:Qt6_DIR = "$qtRoot\lib\cmake\Qt6"
$env:Path += ";$qtRoot\bin"
```

> **VS Code 用户必读**：仓库已包含共享的 `.vscode/` 配置（`c_cpp_properties.json` / `settings.json` / `tasks.json` / `launch.json`），全部通过 `${env:QT_DIR}` 与 `${env:QT_MINGW_BIN}` 引用上述环境变量。设置好这两个变量后重启 VS Code，即可解决 `无法打开源文件 QApplication` 等 IntelliSense 报错，并获得一键构建/测试/调试。

验证 Qt 命令可用：

```powershell
qmake --version
```

### 4. 克隆仓库并构建

```powershell
Set-Location E:\dev\my-schoolworks\26-spring-cpp-group

git clone https://github.com/nick-cjyx9/26-spring-cpp-group.git
cd campus-rpg

# 创建构建目录并配置（MinGW Makefiles + Qt 6.11.1 mingw_64）
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="E:/Qt/6.11.1/mingw_64" `
    -G "MinGW Makefiles" `
    -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"

# 编译
cmake --build build --config Release --parallel
```

> 如果你用的是 **Visual Studio 生成器**，去掉 `-G` 与 `-DCMAKE_CXX_COMPILER`，可执行文件会生成在 `build\Release\` 与 `build\tests\Release\` 下（MinGW 生成器则直接放在 `build\` 与 `build\tests\`）。

### 5. 运行程序

```powershell
# 运行主程序（MinGW 生成器路径；VS 生成器改为 .\build\Release\CampusRPG.exe）
.\build\CampusRPG.exe

# 如果提示找不到 Qt 动态库，先执行 windeployqt
windeployqt .\build\CampusRPG.exe
```

### 6. 运行单元测试

```powershell
# 方法 1：通过 ctest
ctest --test-dir build -C Release --output-on-failure

# 方法 2：直接运行测试可执行文件（MinGW 生成器路径）
.\build\tests\CampusRPGTests.exe
```

测试采用**纯 C++ 轻量运行器**（`tests/test_core.cpp`），不依赖 Qt/SQLite，也不依赖事件循环，因此可在任意命令行环境（含无头 CI）中运行。运行器在失败时不会中止整个测试套件（等价于 `QCOMPARE`/`QVERIFY` 语义），并打印 `run: N  failed: M` 汇总、以非零退出码上报 CTest。

## 三、IDE 配置

### Qt Creator（推荐）

1. 打开 `CMakeLists.txt`
2. 选择 Qt 6 工具包（Kits）
3. 点击 “Configure Project”
4. 直接点击左下角 ▶️ 运行

### Visual Studio

```powershell
# 生成 VS 2022 工程文件
cmake -B build-vs -S . -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH=$env:QT_DIR

# 用 VS 打开
cmake --open build-vs
```

### VS Code

仓库已自带共享的 `.vscode/` 配置（IntelliSense、构建/测试任务、gdb 调试）。首次打开会提示安装推荐扩展（C/C++、CMake Tools）。

**前提**：先按 §2.3 设置好用户环境变量 `QT_DIR` 与 `QT_MINGW_BIN`，然后**重启 VS Code**。配置内部全部用 `${env:QT_DIR}` / `${env:QT_MINGW_BIN}` 引用，无需任何机器相关路径。

快捷操作：

- `Ctrl+Shift+B` — 构建（默认任务 `cmake-build`）
- `F5` — 调试（选 “Debug CampusRPG” 或 “Debug CampusRPGTests”）
- 命令面板运行任务 `cmake-test` 运行单元测试

> 如需自定义，可改本地未提交的设置，但不要把机器相关路径回填进共享配置。

## 四、团队同步检查清单

每位成员在第一次拉取代码后，请按顺序确认：

```powershell
# 1. Git 可用
git --version

# 2. CMake 版本 >= 3.20
cmake --version

# 3. 环境变量 QT_DIR / QT_MINGW_BIN 已设置（VS Code 与构建脚本依赖）
$env:QT_DIR; $env:QT_MINGW_BIN
Test-Path "$env:QT_DIR/include/QtWidgets"
Test-Path "$env:QT_MINGW_BIN/g++.exe"

# 4. Qt 能被 CMake 找到
cmake -E env QT_DIR=$env:QT_DIR cmake --find-package -DNAME=Qt6 -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST

# 5. 能成功构建并运行测试
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH=$env:QT_DIR -G "MinGW Makefiles" `
    -DCMAKE_CXX_COMPILER="$env:QT_MINGW_BIN/g++.exe"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## 五、项目结构

```text
26-spring-cpp-group/
├── .github/workflows/ci.yml   # GitHub Actions 持续集成（须在仓库根目录）
├── .vscode/                   # 共享的 VS Code 配置（IntelliSense/任务/调试）
│   ├── c_cpp_properties.json
│   ├── settings.json
│   ├── tasks.json
│   └── launch.json
├── campus-rpg/
│   ├── src/
│   │   ├── core/              # 核心领域模型（纯 C++，无 Qt/SQLite 依赖，单元测试目标）
│   │   ├── data/              # SQLite 数据库管理层（QtSql）
│   │   ├── manager/           # GameManager 总控
│   │   ├── ui/                # Qt 界面（主窗口 + 各功能页面）
│   │   └── main.cpp           # 程序入口
│   ├── tests/                 # 纯 C++ 单元测试（轻量运行器 + CTest）
│   ├── resources/             # 图标、资源文件
│   └── CMakeLists.txt         # CMake 主配置
├── AGENTS.md
├── README.md                  # 本文件
└── scripts_cheat_sheet.md
```

## 六、Git 协作规范

```powershell
# 主分支保护，个人开发在 feature 分支
git checkout -b feature/character-page

# 提交前先在本地构建测试通过
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

git add .
git commit -m "feat: add character page UI"
git push -u origin feature/character-page
```

## 七、参考项目

本项目骨架参考了以下开源项目：

- [euler0/mini-cmake-qt](https://github.com/euler0/mini-cmake-qt) — Qt 6 + CMake 模板
- [awaszeciak/object-oriented-rpg](https://github.com/awaszeciak/object-oriented-rpg) — C++ OOP RPG 结构参考
- [mng-25/DungeonManager](https://github.com/mng-25/DungeonManager) — Qt + SQLite 持久化参考
