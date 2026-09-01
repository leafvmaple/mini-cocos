# zocos

**zocos** 是为学习 [cocos2d-x](https://github.com/cocos2d/cocos2d-x) 而**自研**的极简 2D 引擎示例：用 GLFW + OpenGL 3.3 Core / Vulkan 1.0 实现与 cocos2d-x 相近的概念（`Director`、`Scene`、`Node`、`Sprite`），便于对照官方引擎理解场景图、变换与渲染循环。仅供学习阅读，不面向生产。

命名与约定见仓库根目录 [`CONVENTIONS.md`](CONVENTIONS.md)（供人类与 AI 助手统一遵循）。

## Features

- GLFW 窗口与可切换的 **OpenGL 3.3 Core / Vulkan 1.0** 渲染后端
- 正交 2D 投影（Y 轴向上，帧缓冲左下角为原点）
- 场景图：父子节点、局部变换（位置、缩放、旋转、锚点、内容尺寸）
- 带纹理的四边形；可选 PNG/JPEG 等（[stb_image](https://github.com/nothings/stb)，`third_party/stb_image.h`）
- 文本渲染：`Label` 使用 TTF/OTF 字体（`stb_truetype`）动态栅格化；仓库内置免费 Noto 字体（`fonts/`）
- 资源缓存：`TextureCache` 复用纹理并按引用计数自动释放 GPU 资源
- 帧动画：`Animation` + `Animate`（可通过 `setTextureRect` 切换图集帧）
- 缓动动作：`ActionEase` 包裹任意 `ActionInterval`，内置 `EaseSine*` / `EaseCubic*`（In/Out/InOut）曲线，以及带 `rate` 指数的乘幂缓动 `EaseIn` / `EaseOut` / `EaseInOut`
- Lua 脚本导出系统（`cc.*`）：可在 Lua 中创建 `Director` / `Scene` / `Sprite` / `Label` 并驱动 `Action`
- 场景管理：`Director` 支持 `replaceScene` 淡入淡出，以及可暂停/恢复 Action 和 Scheduler 的 `pushScene` / `popScene` 场景栈
- 示例：Lua 脚本中的精灵轨道运动与自转（或通过命令行传入图片路径）

## Requirements

- **CMake** 3.16+
- **C++17** 编译器
- 支持 **OpenGL 3.3** 或 **Vulkan 1.0** 的显卡驱动（只需其中一种即可运行对应后端）
- 首次配置需联网（CMake **FetchContent** 拉取 GLFW 3.4 与 Lua 5.4.6）
- Vulkan SDK 可选；找不到 SDK 时，CMake 会自动拉取 Vulkan Loader 与 Headers

## Build

渲染后端在配置时通过 `ZOCOS_RENDER_API` 选择。建议为不同后端使用独立构建目录，避免在同一目录中切换缓存；当前默认后端是 Vulkan。

```bash
# OpenGL 3.3（依赖少，适合先阅读渲染流程）
cmake -S . -B build-opengl -DZOCOS_RENDER_API=OPENGL
cmake --build build-opengl --config Release

# Vulkan 1.0（未安装 Vulkan SDK 时会自动拉取编译依赖）
cmake -S . -B build-vulkan -DZOCOS_RENDER_API=VULKAN
cmake --build build-vulkan --config Release
```

在 Windows（Visual Studio 生成器）下，可执行文件一般在：

`build-opengl/Release/zocos.exe` 或 `build-vulkan/Release/zocos.exe`。使用 Ninja 等单配置生成器时，可执行文件直接位于对应构建目录下。

在类 Unix 系统上：

`build-opengl/zocos` 或 `build-vulkan/zocos`

### 可选：freestanding STL（zstl）

引擎数据结构/算法不直接写 `std::`，而是通过 `mstd` 别名（见 `src/base/ZCStd.h`）。默认 `mstd = std`（host libstdc++/MSVC STL）；打开 **`-DZOCOS_USE_SYS_STL=ON`** 后切换为 `mstd = sys`，由子模块 `third_party/zstl` 提供的 freestanding 实现支撑——可在没有宿主 C++ 标准库的环境中构建。

```bash
cmake -B build-sys -DZOCOS_USE_SYS_STL=ON
cmake --build build-sys --config Release
```

该配置已纳入 CI（`sys-stl` 任务）：在 zstl 后端下构建引擎并运行同一套单元测试，确保这条路径不会悄悄失效。

### clangd（Cursor / VS Code）

仓库已包含 **`.clangd`** 与 **`.vscode/settings.json`** 中的 clangd 选项：优先使用仓库根目录的 **`compile_commands.json`**（与真实编译参数一致，含 FetchContent 的 GLFW 头路径）。

1. 安装扩展：**Clangd**（`llvm-vs-code-extensions.vscode-clangd`），若用 CMake 配置工程可再装 **CMake Tools**（见 `.vscode/extensions.json` 推荐列表）。
2. 生成 clangd 专用编译数据库（推荐，**不影响你现有 build 目录**）：

   ```bash
   .\gen_compile_commands.bat
   ```

   该脚本会在 `build-clangd/` 下执行 CMake（Ninja）并把 `compile_commands.json` 复制到仓库根目录，供 clangd 索引。若系统 `PATH` 里没有 Ninja，会自动尝试使用 Visual Studio 自带 Ninja；若当前终端缺少 `cl.exe` 环境，也会自动尝试初始化 VS 构建环境。

3. 也可手动配置并生成编译数据库（**Ninja** 在 Windows 上会生成 `compile_commands.json`，利于 clangd）：

   ```bash
   cmake -S . -B build-clangd -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   cmake --build build-clangd
   ```

4. 若使用 **CMake Tools**，已设置 `cmake.copyCompileCommands`，配置成功后会将 `compile_commands.json` 复制到仓库根目录，便于工具查找。
5. 纯 **Visual Studio 生成器** 在部分 CMake 版本下可能不产生 `compile_commands.json`，此时 clangd 会回退到项目根目录的 **`.clangd`**（仅含 `-Isrc` 等；GLFW 路径可能不全）。需要完整索引时请改用 **Ninja** 或升级 CMake。

> 为什么之前“加了 json 后反而编译不过”？
>
> 常见原因不是 json 本身，而是把 clangd 配置“强写”进真实构建链，例如：
>
> - 强制 `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER` 改为 clang（破坏原有 MSVC 工具链）
> - 把 clangd 专用参数写进全局 `target_compile_options`
> - 复用同一个 `build/` 目录切换生成器（Visual Studio 和 Ninja 混用）
>
> 本仓库现在采用“clangd 专用 build 目录 + 根目录 compile_commands.json”的方式，避免影响正常编译。

### clang-format

根目录 **`.clang-format`** 约定 **4 空格**缩进（基于 LLVM，列宽 100）。**`.clang-format-ignore`** 排除 `third_party/`。工作区已开启 **`editor.formatOnSave`**，保存时由 **Clangd** 按 `.clang-format` 格式化 C/C++。

在终端批量格式化（需已安装 LLVM，且 `clang-format` 在 `PATH` 中）：

```bash
# 示例：仅格式化 src（PowerShell）
Get-ChildItem -Path src -Recurse -Include *.cpp,*.h | ForEach-Object { clang-format -i $_.FullName }
```

## Run

```bash
# 内置棋盘格纹理
./build-opengl/Release/zocos.exe   # Windows + Visual Studio 生成器示例
./build-opengl/zocos               # Ninja / Linux / macOS 示例

# 图片（stb_image 支持的常见格式）
./build-opengl/zocos path/to/image.png
```

默认会执行 `scripts/main.lua`，CMake 在构建后会自动把 `scripts/` 目录复制到可执行文件同级目录。

## Tests

仓库带有一组**无窗口/无 GPU 依赖**的单元测试（`tests/`），覆盖数学（`Mat4`/正交投影/节点局部矩阵）、UTF-8 解码、`Ref` 引用计数与 autorelease 池，以及 `Renderer` 的排序/合批逻辑（用 fake device 验证同纹理合并、精灵合批）。测试目标 `zocos_tests` 只编译自包含的引擎源文件，不拉入 GL/Vulkan/窗口代码。

```bash
cmake -B build
cmake --build build --config Debug --target zocos_tests
ctest --test-dir build -C Debug --output-on-failure
```

测试默认开启，可用 `-DZOCOS_BUILD_TESTS=OFF` 关闭。CI（GitHub Actions，`.github/workflows/ci.yml`）会在 **OpenGL 与 Vulkan 两个后端**下分别构建并运行测试。

## Project layout（对齐 cocos2d-x 模块划分）

| Path | Role |
| ------ | ------ |
| `src/math/ZCMath.h` | `Vec2`、`Size`、`Mat4`（文件名避免与系统 `Math.h` 在大小写不敏感盘上冲突） |
| `src/base/ZCDirector.*` | 主循环、GL 上下文、投影、当前场景（类型为 `Director`） |
| `src/base/ZCTextureCache.*` | 纹理资源缓存与 GPU 纹理生命周期管理（类型为 `TextureCache`） |
| `src/2d/ZCNode.*` | 场景图基类：`visit`、`updateTree`、变换（类型为 `Node` / `Scene`） |
| `src/2d/ZCSprite.*` | 纹理四边形与简单着色器（类型为 `Sprite`） |
| `src/base/ZCAction.*` | 基础动作系统（含 `Animation` / `Animate` 帧动画） |
| `src/scripting/ZCLuaEngine.*` / `src/scripting/ZCLuaManual.*` | Lua 引擎封装、统一导出入口（`register_all_zocos*`）与手动导出 |
| `src/platform/opengl/` | OpenGL 3.3 View、最小函数入口与 RenderDevice |
| `src/platform/vulkan/` | Vulkan View、交换链、资源与 RenderDevice |
| `src/main.cpp` | 引擎入口（加载并执行 Lua demo 脚本） |
| `scripts/main.lua` | Lua demo 场景脚本 |
| `.clang-format` / `.clang-format-ignore` | 代码风格（4 空格）；忽略第三方目录 |
| `third_party/stb_image.h` | 纹理加载 |
| `CONVENTIONS.md` | 项目命名与设计约定 |

头文件以 **`src` 为 include 根**，写法类似 cocos：`#include "math/ZCMath.h"`、`#include "2d/ZCNode.h"` 等。

## License

本仓库中的引擎示例代码可自由用于学习、修改。

`stb_image.h` 遵循其文件头中的许可说明。
