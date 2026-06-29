# LearnOpenGLCN 工程文档

本目录记录 LearnOpenGLCN 的代码架构、模块边界、编码约定和扩展方式。

工程的长期目标是系统学习并整理 LearnOpenGLCN 网站中的全部内容，同时把部分学习分支扩展成可复用功能模块，例如使用 Qt + OpenGL 显示工业相机图像、用 OpenGL 绘制三维轨迹或点云画面。

工程正在从教程示例集合逐步演进为采用整洁架构（Clean Architecture）的 OpenGL 学习与功能实验项目。文档会明确区分“当前实现”和“目标架构”：当前代码尚未具备的能力不会被描述成已经完成。前期允许在 `ui` 层先跑通 Qt/OpenGL 原型，随后再按 domain/application/infrastructure/ui/app 分层拆分。

## 文档索引

- [代码架构与依赖关系](./ARCHITECTURE.md)：当前结构、目标分层、依赖规则及各模块职责。
- [编码规范](./CODING_STYLE.md)：C++、OpenGL、CMake、资源和注释约定。
- [扩展指南](./EXTENDING.md)：新增课程、基础设施适配器和架构演进的推荐步骤。
- [Codex 上下文](../.agents/CODEX_CONTEXT.md)：记录当前长期目标、阶段性决策和最近上下文，避免后续对话过长后丢失背景。

## 当前工程状态

当前工程使用 CMake 3.21+、C++17、Ninja 和 MSVC 构建，主要依赖 OpenGL、GLFW、GLAD、GLM、stb_image 与 Qt Widgets/OpenGLWidgets。

现有主要构建链路为：

```text
LearnOpenGLCN_Lessons (executable)
    └── lessons (static library)
        ├── infrastructure (static library)
        │   ├── glad
        │   ├── glfw3
        │   ├── OpenGL::GL
        │   └── glm::glm
        └── stb_image

LearnOpenGLCN_Qt (executable)
    └── learnopengl_ui (static library)
        ├── Qt6::Widgets
        ├── Qt6::OpenGLWidgets
        ├── Qt6::OpenGL
        ├── glad
        ├── OpenGL::GL
        └── stb_image
```

`ui/` 已形成 `learnopengl_ui` target，并承载当前 Qt/OpenGL 显示原型。`domain/` 与 `application/` 已建立目录，但尚未形成对应 CMake target。它们是后续整洁架构迁移的目标位置，不代表当前已经完成分层。

## 构建入口

推荐在 Windows PowerShell 中使用仓库根目录下的脚本初始化 MSVC 环境并构建：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
```

Release 构建：

```powershell
.\msvc-cmake.ps1 -Config Release -NoPause
```

构建产物默认位于：

```text
out/build/<preset>/bin/LearnOpenGLCN_Qt.exe
out/build/<preset>/bin/LearnOpenGLCN_Lessons.exe
```

Debug 构建后运行 Qt 原型：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Qt.exe
```

Debug 构建后运行课程代码：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe transform
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe --list
```
