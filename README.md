# zocos

**zocos** 是为学习 [cocos2d-x](https://github.com/cocos2d/cocos2d-x) 而**自研**的极简 2D 引擎示例：用 GLFW + OpenGL 3.3 Core 实现与 cocos2d-x 相近的概念（`ZCDirector`、`ZCScene`、`ZCNode`、`ZCSprite`），便于对照官方引擎理解场景图、变换与渲染循环。仅供学习阅读，不面向生产。

命名与约定见仓库根目录 [`CONVENTIONS.md`](CONVENTIONS.md)（供人类与 AI 助手统一遵循）。

## Features

- GLFW 窗口与 **OpenGL 3.3 Core**（通过 `glfwGetProcAddress` 加载函数指针）
- 正交 2D 投影（Y 轴向上，帧缓冲左下角为原点）
- 场景图：父子节点、局部变换（位置、缩放、旋转、锚点、内容尺寸）
- 带纹理的四边形；可选 PNG/JPEG 等（[stb_image](https://github.com/nothings/stb)，`third_party/stb_image.h`）
- 示例：旋转棋盘格（或通过命令行传入图片路径）

## Requirements

- **CMake** 3.16+
- **C++17** 编译器
- **OpenGL 3.3** 驱动
- 首次配置需联网（CMake **FetchContent** 拉取 GLFW 3.4）

## Build

```bash
cmake -B build
cmake --build build --config Release   # MSVC: 使用 --config Release
```

在 Windows（Visual Studio 生成器）下，可执行文件一般在：

`build/Release/zocos.exe`（或 `build/Debug/zocos.exe`）。

在类 Unix 系统上：

`build/zocos`

### clangd（Cursor / VS Code）

仓库已包含 **`.clangd`** 与 **`.vscode/settings.json`** 中的 clangd 选项：优先使用 **`build/compile_commands.json`**（与真实编译参数一致，含 FetchContent 的 GLFW 头路径）。

1. 安装扩展：**Clangd**（`llvm-vs-code-extensions.vscode-clangd`），若用 CMake 配置工程可再装 **CMake Tools**（见 `.vscode/extensions.json` 推荐列表）。
2. 配置并生成编译数据库（**Ninja** 在 Windows 上也会生成 `compile_commands.json`，利于 clangd）：

   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

3. 若使用 **CMake Tools**，已设置 `cmake.copyCompileCommands`，配置成功后会将 `compile_commands.json` 复制到仓库根目录，便于工具查找。
4. 纯 **Visual Studio 生成器** 在部分 CMake 版本下可能不产生 `compile_commands.json`，此时 clangd 会回退到项目根目录的 **`.clangd`**（仅含 `-Isrc` 等；GLFW 路径可能不全）。需要完整索引时请改用 **Ninja** 或升级 CMake。

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
./build/Release/zocos.exe          # Windows 示例路径
./build/zocos                      # Linux / macOS

# 图片（stb_image 支持的常见格式）
./build/zocos path/to/image.png
```

## Project layout（对齐 cocos2d-x 模块划分）

| Path | Role |
|------|------|
| `src/math/ZCMath.h` | `ZCVec2`、`ZCSize`、`ZCMat4`（文件名避免与系统 `Math.h` 在大小写不敏感盘上冲突） |
| `src/base/ZCDirector.*` | 主循环、GL 上下文、投影、当前场景（对应引擎 `base` 层） |
| `src/2d/ZCNode.*` | 场景图基类：`visit`、`updateTree`、变换 |
| `src/2d/ZCSprite.*` | 纹理四边形与简单着色器 |
| `src/platform/opengl_loader.*` | 最小 GL 3.3 入口（平台 / 图形后端相关） |
| `src/main.cpp` | 示例入口 |
| `.clang-format` / `.clang-format-ignore` | 代码风格（4 空格）；忽略第三方目录 |
| `third_party/stb_image.h` | 纹理加载 |
| `CONVENTIONS.md` | 项目命名与设计约定 |

头文件以 **`src` 为 include 根**，写法类似 cocos：`#include "math/ZCMath.h"`、`#include "2d/ZCNode.h"` 等。

## License

本仓库中的引擎示例代码可自由用于学习、修改。

`stb_image.h` 遵循其文件头中的许可说明。
