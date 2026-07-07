# 备忘录

## 渲染 mermaid 图表

```sh
npx -y @mermaid-js/mermaid-cli -i UML类图.mmd -o UML类图.svg -b white
```

---

## 构建与测试工具链脚本（PowerShell）

以下命令均在 `campus-rpg/` 目录下执行。

### 0. 一次性环境准备（仅首次）

本机无 Qt 自带 MinGW，改用 MSYS2 ucrt64 工具链（g++ 15.2.0，支持 C++20）。首次需通过 MSYS2 安装 Ninja（Windows 上用宽字符 API，能正确处理含中文的仓库路径）：

```powershell
& "C:\msys64\usr\bin\pacman.exe" -S --noconfirm --needed mingw-w64-ucrt-x86_64-ninja
```

每次开新终端构建前，把 ucrt64 加入 PATH（gcc 运行时 DLL 在此目录）：

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
```

> 工具链位置：`C:\msys64\ucrt64\bin\{gcc,g++,ninja}.exe`
> 不要用旧版 `mingw32-make`（Dev-Cpp 4.9.2 / GNU Make 3.82）——它走窄 API，会把含中文的路径按 GBK 解码搞乱，导致 windres/gcc 找不到文件。

### 1. 完整配置 + 编译

使用 Ninja 生成器，SFML / SQLite3 通过 FetchContent 自动下载。SFML 采用**静态构建**并关闭 audio 模块（规避 SFML 自带旧 libFLAC.a 与新版 mingw 运行时的 `__imp___iob_func` 链接冲突，以及 windres 编译 `.rc` 时的中文路径问题）；同时静态链接 mingw 运行时（libgcc/libstdc++/libwinpthread），生成的 `CampusRPG.exe` 仅依赖 Windows 系统 DLL，可双击直接运行：

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -G "Ninja" `
    -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe" `
    -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe" `
    -DCMAKE_MAKE_PROGRAM="C:/msys64/ucrt64/bin/ninja.exe" `
    -DSFML_BUILD_AUDIO=OFF `
    -DBUILD_SHARED_LIBS=OFF `
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -static-libgcc -static-libstdc++" `
    -DCMAKE_C_FLAGS_RELEASE="-O2 -static-libgcc" `
    -DCMAKE_EXE_LINKER_FLAGS="-static"
cmake --build build --config Release --parallel
```

> Ninja 为单配置：可执行文件生成在 `build\` 与 `build\tests\`。
> `CampusRPG.exe` 完全静态：不依赖 SFML/libstdc++/libgcc 任何 DLL，仅依赖 Windows 自带的 KERNEL32/GDI32/OPENGL32/UCRT 等，**可在任意机器双击运行**。
> Visual Studio 生成器（多配置）下产物在 `build\Release\` 与 `build\tests\Release\`。

### 2. 运行单元测试

```powershell
ctest --test-dir build -C Release --output-on-failure
```

或直接运行测试可执行文件（Ninja 生成器路径）：

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

静态构建无需 SFML DLL；运行时资源（textures/fonts）已通过 post-build 命令复制到 `build\resources\`。

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
# MSYS2 ucrt64 工具链（构建必需）
Test-Path "C:/msys64/ucrt64/bin/g++.exe"   # g++ 15.2.0 (C++20)
Test-Path "C:/msys64/ucrt64/bin/ninja.exe" # Ninja 构建工具
& "C:/msys64/ucrt64/bin/g++.exe" --version
```

### 7. 一键完整验证

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release `
    -G "Ninja" `
    -DCMAKE_C_COMPILER="C:/msys64/ucrt64/bin/gcc.exe" `
    -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe" `
    -DCMAKE_MAKE_PROGRAM="C:/msys64/ucrt64/bin/ninja.exe" `
    -DSFML_BUILD_AUDIO=OFF `
    -DBUILD_SHARED_LIBS=OFF `
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -static-libgcc -static-libstdc++" `
    -DCMAKE_C_FLAGS_RELEASE="-O2 -static-libgcc" `
    -DCMAKE_EXE_LINKER_FLAGS="-static"
if ($?) { cmake --build build --config Release --parallel }
if ($?) { ctest --test-dir build -C Release --output-on-failure }
if ($?) { cd build; .\CampusRPG.exe }
```
