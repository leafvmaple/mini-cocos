# mini-cocos

A minimal 2D engine skeleton modeled after **cocos2d-x** concepts: `Director`, `Scene`, `Node`, and `Sprite`. Intended for reading the code to understand scene graphs, transforms, and a simple render loop—not for production use.

## Features

- GLFW window and **OpenGL 3.3 Core** (function pointers loaded via `glfwGetProcAddress`)
- Orthographic 2D projection (Y-up, origin at bottom-left of the framebuffer)
- Scene graph: parent–child nodes, local transform (position, scale, rotation, anchor, content size)
- Textured sprite quad; optional PNG/JPEG via [stb_image](https://github.com/nothings/stb) (`third_party/stb_image.h`)
- Demo: rotating checkerboard (or image from command line)

## Requirements

- **CMake** 3.16 or newer
- **C++17** compiler
- **OpenGL 3.3** drivers
- Network on first configure (CMake **FetchContent** downloads GLFW 3.4)

## Build

```bash
cmake -B build
cmake --build build --config Release   # MSVC: add --config Release
```

On Windows with Visual Studio generators, the executable is typically:

`build/Release/mini-cocos.exe` (or `build/Debug/mini-cocos.exe`).

On Unix-like systems:

`build/mini-cocos`

## Run

```bash
# Built-in checkerboard texture
./build/Release/mini-cocos.exe          # Windows example path
./build/mini-cocos                      # Linux / macOS

# RGBA image (stb_image supports common formats)
./build/mini-cocos path/to/image.png
```

## Project layout

| Path | Role |
|------|------|
| `src/Director.*` | Main loop, GL context, projection, current scene |
| `src/Node.*` | Scene graph base: `visit`, `updateTree`, transform |
| `src/Sprite.*` | Textured quad + simple shader |
| `src/MiniMath.h` | Vectors and column-major `Mat4` (named **MiniMath** so `Math.h` does not shadow system `<math.h>` on case-insensitive filesystems) |
| `src/opengl_loader.*` | Minimal GL 3.3 entry points |
| `third_party/stb_image.h` | Texture loading |

## License

Engine sample code in this repository: use and modify freely for learning.

`stb_image.h` follows its own license (see file header in `third_party/stb_image.h`).
