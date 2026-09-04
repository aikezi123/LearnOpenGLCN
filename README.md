# EngineeringLab 工程文档

EngineeringLab 是个人 C++ 工程技术持续学习与实验平台。工程以可运行、可验证、可逐步演进为目标，当前包含 LearnOpenGL 课程、Qt/OpenGL 综合工作台、工业相机、轨迹算法和并发组件；后续可以继续学习 CAD、机器人、图形学、CUDA、IPC 等内容。

这些名称表示学习方向，不直接等同于顶层代码模块。源码继续按职责分为 domain、application、infrastructure、ui 和 composition_root；只有真实依赖或复用边界出现时，才拆分新的目录、端口和 CMake target。

## 文档索引

### 架构

- [代码架构与依赖关系](./DOC/architecture/ARCHITECTURE.md)：项目定位、当前结构、依赖规则和目标边界。

### 模块专题

- [相机采集与 OpenGL 显示链路](./DOC/modules/CAMERA_ARCHITECTURE.md)
- [线程池并发能力](./DOC/modules/THREAD_POOL.md)
- [日志模块](./DOC/modules/LOGGING.md)
- [二维阿基米德螺旋轨迹](./DOC/modules/TRAJECTORY_2D.md)

### 开发指南

- [编码规范](./DOC/guides/CODING_STYLE.md)
- [扩展指南](./DOC/guides/EXTENDING.md)
- [自动化测试](./DOC/guides/TESTING.md)

### Agent 协作入口

- [项目协作规则](./.agents/AGENTS.md)
- [当前上下文](./.agents/CODEX_CONTEXT.md)

## 架构定位

稳定依赖方向为：

```text
composition_root ──> ui
composition_root ──> infrastructure
composition_root ──> application

ui ────────────────> application ──> domain
infrastructure ────> application ──> domain
```

各层职责：

- `domain/`：纯数据、数学模型和算法，例如图像帧、轨迹和以后真实需要的几何模型。
- `application/`：用例、流程和外部能力端口，例如相机采集流程。
- `infrastructure/`：OpenGL、CUDA、IPC、设备 SDK、并发和文件系统等具体技术实现。
- `ui/`：Qt 界面、交互和有效图形上下文中的显示行为。
- `composition_root/`：选择具体实现、创建对象并注入依赖。
- `lessons/`：保持可运行、可对照教程的课程代码，不要求为教学形式强套业务分层。
- `tools/`：独立命令行工具。

CAD、机器人、视觉和图形学可以同时使用多个层与能力目录，不预先建立对应的顶层模块。

## 当前 CMake target

```text
EngineeringWorkbench (executable)
    ├── englab::ui
    ├── englab::application
    ├── englab::camera_galaxy
    └── englab::logging

OpenGLLessons (executable)
    └── englab::opengl_lessons
        ├── englab::graphics_opengl
        ├── glad
        ├── glfw3
        ├── OpenGL::GL
        ├── glm::glm
        └── stb_image

englab::ui
    ├── englab::application
    ├── englab::domain
    └── Qt/OpenGL UI dependencies

englab::camera_galaxy
    ├── englab::application
    └── Galaxy::SDK

englab::application
    ├── englab::domain
    └── englab::diagnostics

englab::diagnostics
    └── fmt::fmt（vcpkg，INTERFACE）

englab::concurrency
    └── Threads::Threads

englab::logging
    ├── englab::diagnostics
    └── spdlog::spdlog（vcpkg，PRIVATE）
```

OpenGL 课程不再链接包含相机 SDK 和并发实现的聚合 infrastructure target。OpenGL、Galaxy 相机、线程池和日志分别由 `englab::graphics_opengl`、`englab::camera_galaxy`、`englab::concurrency`、`englab::logging` 表达技术依赖。综合工作台已创建进程级日志后端并注入相机应用服务，当前尚未写业务日志。

C++ 项目代码统一使用 `engineeringlab` 根命名空间；CMake alias 使用较短的 `englab::` 前缀。

## 当前功能

- `domain/image` 提供 `ImageFrame` / `PixelFormat`。
- `domain/trajectory` 提供二维阿基米德螺旋纯算法。
- `application/camera` 提供 `ICameraDevice` 和 `CameraCaptureService`。
- `infrastructure/camera/galaxy` 提供大恒相机适配器。
- `infrastructure/concurrency` 提供固定线程池。
- `application/diagnostics` 提供日志端口和 `ModuleLogger`；`infrastructure/logging` 提供 spdlog 实现；综合工作台已将唯一后端注入 `CameraCaptureService`，当前尚未写业务日志。
- `infrastructure/shader` 提供当前课程使用的 OpenGL Shader 封装。
- `ui` 提供相机预览、图像观察变换、轨迹生成与导出页面。
- `lessons` 保留 LearnOpenGL 课程注册和 GLFW 课程实现。

## 构建入口

fmt 12.1.0、spdlog 1.17.0 和 GoogleTest/GoogleMock 1.17.0 由根目录 `vcpkg.json` 统一管理，版本通过 `builtin-baseline` 固定。构建前需安装 vcpkg 并设置 `VCPKG_ROOT` 环境变量；CMake preset 会使用其 toolchain 自动恢复依赖。Windows preset 使用 `x64-windows-static-md`，即静态第三方库和动态 MSVC CRT。`vcpkg_installed/` 与依赖构建缓存不提交仓库。

Debug：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
```

Release：

```powershell
.\msvc-cmake.ps1 -Config Release -NoPause
```

测试：

```powershell
ctest --preset ninja-msvc-debug --output-on-failure
```

运行综合工作台：

```powershell
.\out\build\ninja-msvc-debug\bin\EngineeringWorkbench.exe
```

运行 OpenGL 课程：

```powershell
.\out\build\ninja-msvc-debug\bin\OpenGLLessons.exe
.\out\build\ninja-msvc-debug\bin\OpenGLLessons.exe transform
.\out\build\ninja-msvc-debug\bin\OpenGLLessons.exe --list
```

ASan preset 使用 `ENGINEERINGLAB_ENABLE_ASAN`，构建产物仍位于 `out/build/<preset>/bin`。
