# zocos 项目约定（给贡献者与 AI）

阅读或修改本仓库前，请先遵守以下规范，便于与 [cocos2d-x](https://github.com/cocos2d/cocos2d-x) 的学习对照保持一致。

## 项目名称

- 工程与可执行文件名称：**zocos**（CMake `project(zocos)`，目标名 `zocos`）。

## 命名空间

- 引擎与数学类型均在 **`namespace zocos`** 中声明。
- 不在全局命名空间添加与引擎同名的类型，避免与第三方或系统头冲突。

## 类名：无前缀（通过命名空间区分）

cocos2d-x 历史 API 以 **`CC`** 为前缀（如 `CCNode`、`CCDirector`）。本项目中，**公开引擎类型**不再使用 `ZC` 前缀，而是依赖 `namespace zocos` 完成区分：

| 概念 | zocos 类型 |
| ------ | ------------ |
| 导演 / 主循环 | `Director` |
| 场景 | `Scene` |
| 节点 | `Node` |
| 精灵 | `Sprite` |

新增同类概念时，继续使用 **PascalCase**（例如未来的 `Label`）。

> 说明：**文件名**仍保持 `ZC*.h/.cpp`（例如 `ZCDirector.h`）以兼容现有目录结构。

## 数学与辅助类型

- 向量、尺寸、矩阵：`Vec2`、`Size`、`Mat4`（定义于 `src/math/ZCMath.h`）。
- 与节点局部矩阵相关的自由函数：`zcNodeLocalMatrix`（小写 `zc` 前缀 + PascalCase，表示非类成员工具函数）。

## 目录与源文件（对齐 cocos2d-x）

- **`src/math/`**：数学与几何（仅头文件 `ZCMath.h` 亦可）。
- **`src/base/`**：导演、应用级生命周期（`Director`）。
- **`src/2d/`**：二维节点与渲染对象（`Node`、`Sprite`、`Scene`）。
- **`src/platform/`**：平台与图形 API 封装（此处为 `ZCOpenGLLoader`）。
- **`src/main.cpp`**：可执行入口、示例代码。

源文件命名保持 `ZC` 前缀：`ZCDirector.cpp/.h`、`ZCNode.cpp/.h` 等。

**Include 规则**：`target_include_directories` 将 **`src`** 作为根目录，与 cocos 类似：

- `#include "math/ZCMath.h"`
- `#include "base/ZCDirector.h"`
- `#include "2d/ZCNode.h"`
- `#include "2d/ZCSprite.h"`
- `#include "platform/ZCOpenGLLoader.h"`

勿使用裸 `Math.h` 作文件名，以免在 Windows 等大小写不敏感盘上遮蔽 `<math.h>`。

## 与 cocos2d-x 的对应关系（心智模型）

- `Director::getInstance` ≈ 单例入口与主循环。
- `Scene` 作为根节点承载一层 UI/游戏对象。
- `Node` 负责父子关系、变换与遍历。
- `Sprite` 表示带纹理的矩形绘制。

对照阅读 cocos2d-x 源码时，可将 `CC*` 与本项目同名类型类比，但 **API 不完全一致**，本仓库以实现简单清晰为先。

## 代码格式

- 使用根目录 **`.clang-format`**（4 空格、`third_party/` 在 **`.clang-format-ignore`** 中不格式化）。
- 编辑器侧：工作区已配置保存时格式化；命令行需本机安装 **LLVM** 的 `clang-format`。

## OpenGL 与第三方

- OpenGL 常量与加载方式见 `src/platform/ZCOpenGLLoader.*`；新增 GL 枚举时需核对官方十六进制值，避免笔误。
- `third_party` 下第三方头文件尽量不修改；升级时保留其许可证说明。

## 沟通语言

- AI 助手与用户交流时**一律使用中文**——包括回答、说明、提问与澄清。
- 代码注释、提交信息（commit message）、文档可保持英文（与现有风格一致），但面向用户的对话用中文。

---

**给 AI 助手**：生成或重构代码时，默认使用 **`namespace zocos`**、引擎类名 **无 `ZC` 前缀**（如 `Director` / `Node`）、数学类型 **`Vec2` / `Size` / `Mat4`**；文件名保持 `ZC` 前缀（如 `ZCDirector.h`），新文件放入 **`math/` / `base/` / `2d/` / `platform/`** 等对应目录，include 使用 **`src` 为根的相对路径**，并保持 CMake 目标名 **zocos**。**与用户的所有对话默认使用中文**。
