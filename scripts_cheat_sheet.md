# 备忘录

## 渲染 mermaid 图表

```sh
mmdc -i gantt_chart.mmd -o gantt_chart.png -b white -s 2
```

---

## 构建与测试工具链脚本（PowerShell）

以下命令均在 `campus-rpg/` 目录下执行。

### 1. 完整配置 + 编译

以 MinGW Makefiles + Qt 为例（依赖环境变量 `QT_DIR` / `QT_MINGW_BIN`，见 README §2.3）：

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH=$env:QT_DIR `
    -G "MinGW Makefiles" `
    -DCMAKE_CXX_COMPILER="$env:QT_MINGW_BIN/g++.exe"
cmake --build build --config Release --parallel
```

如果 CMake 找不到 Qt（MSVC 版）：

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
```

> MinGW 生成器为单配置：可执行文件生成在 `build\` 与 `build\tests\`；
> Visual Studio 生成器为多配置：可执行文件生成在 `build\Release\` 与 `build\tests\Release\`。

### 2. 运行单元测试

```powershell
ctest --test-dir build -C Release --output-on-failure
```

或直接运行测试可执行文件（MinGW 生成器路径）：

```powershell
.\build\tests\CampusRPGTests.exe
```

> 测试为纯 C++ 轻量运行器，无 Qt 依赖、无事件循环，可在任意命令行环境运行。

### 3. 运行主程序

```powershell
.\build\CampusRPG.exe
```

（Visual Studio 生成器下为 `.\build\Release\CampusRPG.exe`）

如果提示缺少 Qt DLL：

```powershell
windeployqt .\build\CampusRPG.exe
```

### 4. 清理构建目录

```powershell
Remove-Item -Recurse -Force build
```

### 5. 生成 Visual Studio 工程

```powershell
cmake -B build-vs -S . -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --open build-vs
```

### 6. 环境快速检查

```powershell
git --version
cmake --version
# 环境变量（VS Code 与构建脚本都依赖）
$env:QT_DIR
$env:QT_MINGW_BIN
Test-Path "$env:QT_DIR/include/QtWidgets"
Test-Path "$env:QT_MINGW_BIN/g++.exe"
qmake --version
```

### 7. 一键完整验证

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH=$env:QT_DIR `
    -G "MinGW Makefiles" `
    -DCMAKE_CXX_COMPILER="$env:QT_MINGW_BIN/g++.exe"
if ($?) { cmake --build build --config Release --parallel }
if ($?) { ctest --test-dir build -C Release --output-on-failure }
if ($?) { .\build\CampusRPG.exe }
```
