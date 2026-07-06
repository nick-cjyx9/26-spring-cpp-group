# 备忘录

## 渲染 mermaid 图表

```sh
npx -y @mermaid-js/mermaid-cli -i UML类图.mmd -o UML类图.svg -b white
```

---

## 构建与测试工具链脚本（PowerShell）

以下命令均在 `campus-rpg/` 目录下执行。

### 1. 完整配置 + 编译

以 MinGW Makefiles 为例（SFML / SQLite3 通过 FetchContent 自动下载）：

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
    -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"
cmake --build build --config Release --parallel
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

> 测试为纯 C++ 轻量运行器，无 SFML/SQLite 依赖、无 GUI 事件循环，可在任意命令行环境运行。

### 3. 运行主程序

```powershell
cd build
.\CampusRPG.exe
```

（Visual Studio 生成器下为 `.\build\Release\CampusRPG.exe`）

SFML DLL 已通过 post-build 命令复制到可执行文件目录，无需额外操作。

### 4. 清理构建目录

```powershell
Remove-Item -Recurse -Force build
```

### 5. 生成 Visual Studio 工程

```powershell
cmake -B build-vs -S . -G "Visual Studio 17 2022"
cmake --open build-vs
```

### 6. 环境快速检查

```powershell
git --version
cmake --version
# 环境变量（VS Code 与构建脚本可选依赖）
$env:MINGW_BIN
Test-Path "$env:MINGW_BIN/g++.exe"
Test-Path "$env:MINGW_BIN/gdb.exe"
```

### 7. 一键完整验证

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -G "MinGW Makefiles" `
    -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
    -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"
if ($?) { cmake --build build --config Release --parallel }
if ($?) { ctest --test-dir build -C Release --output-on-failure }
if ($?) { cd build; .\CampusRPG.exe }
```
