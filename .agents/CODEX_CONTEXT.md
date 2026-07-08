# Codex 项目上下文

本文件用于记录 LearnOpenGLCN 当前长期目标、阶段性决策和最近上下文。后续对话过长或上下文被压缩时，Codex 应优先读取本文件，再继续工作。

## 1. 最终目标

LearnOpenGLCN 的最终目标是系统学习并整理 LearnOpenGLCN 网站中的全部内容，并在学习过程中把部分知识点扩展为实际功能模块。

长期方向包括：

- 完成 LearnOpenGLCN 教程中的章节、练习和示例整理。
- 保持教程代码可运行、可对照原教程、可逐步实验。
- 在教程分支上扩展工程能力，例如：
  - 大恒/海康等工业相机实时图像采集与 OpenGL 显示。
  - 三维轨迹数据、点云数据的 OpenGL 绘制与交互展示。
  - Shader、Texture、Buffer、VAO/VBO/EBO、Camera、Transform 等能力的复用封装。

## 2. 架构方向

工程长期采用整洁架构：

```text
composition_root/app
    -> ui
    -> infrastructure
    -> application
    -> domain
```

依赖方向必须指向内层：

- `domain` 不依赖 Qt、OpenGL、GLFW、GLAD、stb_image、相机 SDK 或文件系统适配器。
- `application` 只表达用例和端口，不创建 QWidget/QOpenGLWidget，不调用 `glXXX`，不依赖相机 SDK。
- `infrastructure` 实现具体技术适配，例如 OpenGL 资源、stb_image 图片加载、相机 SDK 适配。
- `ui` 负责 Qt/GLFW 窗口、控件、事件和可视化交互。
- `composition_root` 负责创建对象和注入依赖，不承载业务逻辑。

## 3. 当前阶段策略

当前仍处于学习和原型阶段。为了理解 Qt 如何替代 GLFW、以及 OpenGL 如何在 `QOpenGLWidget` 中运行，允许先在 `ui` 层实现完整功能闭环。

允许的前期做法：

- 在 `QOpenGLWidget` 中直接创建 Shader、VAO/VBO/EBO、Texture。
- 在 UI 层先跑通本地图片纹理显示、相机模拟帧显示、简单点云/轨迹绘制。
- 为了学习过程保留必要解释性注释。

必须记住：

- UI 层大杂烩只是原型，不是最终架构。
- 当功能稳定、重复或需要复用时，应拆分到 application/infrastructure/domain。
- 不要把临时原型写成长期推荐架构。
- 不添加与当前功能无关的预留宏、开关、接口或抽象；只有当前收益明确、后续使用位置清楚时才提前加入。

## 4. Qt 与 OpenGL 当前决策

当前 Qt 已接入 `ui` target，使用 Qt Widgets 和 `QOpenGLWidget`。

最近已经验证：

- 使用 `QImage` / `QOpenGLTexture` 路径曾出现关闭窗口后的 Debug CRT heap corruption。
- 改为 `stb_image + 原生 OpenGL Texture` 后问题消失。
- 当前纹理显示路线应保留为：

```text
stb_image
    -> CPU 像素数据
    -> glTexImage2D / 后续 glTexSubImage2D
    -> OpenGL Texture
    -> QOpenGLWidget 绘制
```

当前本地图片显示使用：

```text
assets/textures/ui/display_image.png
```

当前已建立相机预览的最小分层切片：`domain` 提供 `VideoFrame` / `PixelFormat`，`application` 提供 `ICameraDevice` 和 `CameraPreviewService`，`infrastructure/camera/galaxy` 提供 `GalaxyCameraController` 大恒适配器。大恒 SDK 头文件只出现在 infrastructure 的 `.cpp` 中，不再暴露给 UI 或 application。

当前 Qt 主窗口为左侧 `QTreeWidget` 导航 + 右侧 `QStackedWidget` 页面容器。`composition_root/qt_main.cpp` 创建 `GalaxyCameraController` 并注入 `CameraPreviewService`，`MainWindow` 把 service 传给 `CameraImageCaptureView`，并将该页面注册为“相机模块 / 大恒相机预览”。该窗口通过 Qt queued invoke 把 SDK 线程中的 `domain::VideoFrame` 投递到 UI 线程，再调用提升控件 `DisplayOpenGLImage::setRgb24Frame(...)`。`DisplayOpenGLImage` 在 `paintGL()` 中上传待处理帧，首次或尺寸变化时使用 `glTexImage2D`，后续同尺寸帧使用 `glTexSubImage2D`。相机页面提供水平/垂直翻转、左右 90 度旋转、缩放、平移和重置控件；这些控件只负责 UI 交互，具体显示变换由 `DisplayOpenGLImage` 在绘制前上传 shader uniform 矩阵完成。UI 不再直接处理相机 SDK 类型。

分层代码目录统一采用“层 / 模块 / include + src”的模块优先结构。当前使用 `application/camera/include/camera/...`、`domain/video/include/video/...`、`infrastructure/camera/galaxy/include/camera/galaxy/...`、`infrastructure/shader/include/shader/...`。公共头文件目录保持简洁，不重复嵌套项目名和当前层名。

application 层流程对象统一使用 `Service` 命名，例如 `CameraPreviewService`。端口接口和使用它的 service 放在 application，例如 `ICameraDevice`；它表达 application 为完成流程需要外部系统提供的能力，由 infrastructure 实现。domain 层只放稳定概念和值对象，例如 `VideoFrame`、`PixelFormat`，不放需要驱动外部系统干活的接口。

术语约定：`GalaxyCameraController : ICameraDevice` 这种继承重写叫实现端口；`composition_root` 创建 `GalaxyCameraController`，把它作为 `ICameraDevice` 传给 `CameraPreviewService`，这个从外部传入依赖的动作叫依赖注入。依赖注入不是消除依赖，而是让 service 依赖抽象端口，不直接依赖具体实现。

第三方 SDK 适配器优先使用 Pimpl。公开头文件只前置声明 `Impl` 并持有 `std::unique_ptr<Impl>`；SDK 头文件、SDK 成员、平台句柄和回调类放在 `.cpp` 中，避免大恒/海康等 SDK 类型穿透公共头文件和外层调用方。

相机实时显示的下一阶段建议：

- 首次创建纹理时使用 `glTexImage2D` 分配存储。
- 每帧图像更新使用 `glTexSubImage2D`。
- 不要每帧重新创建 Shader、VAO/VBO/EBO 或 Texture。
- 明确帧宽高、通道数、stride、像素格式和缓冲区所有权。

## 5. 当前工程事实

当前主要目录：

- `composition_root/`：当前可执行程序入口层，包含 Qt 入口 `qt_main.cpp` 和课程入口 `lesson_main.cpp`。
- `ui/`：Qt Widgets / QOpenGLWidget 原型层，已有 `learnopengl_ui` target。
- `lessons/`：LearnOpenGL 教程示例集合。
- `infrastructure/`：现有 OpenGL Shader 等基础设施。
- `domain/`、`application/`：已形成相机预览所需的最小 target，当前包含图像帧模型、相机端口和预览 service。
- `third_party/`：GLAD、GLFW、GLM、stb_image 等第三方依赖。
- `third_party/Galaxy/`：大恒 Galaxy SDK 的第三方依赖包装目录，当前 CMake target 为 `Galaxy::SDK`。现有文件覆盖 VC/C API 的 `GxIAPI`、`DxImageProc`，以及 C++ SDK 的 `GalaxyIncludes.h`、`GxIAPICPPEx.lib`；运行时 DLL 直接放在 `out/build/<preset>/bin`，随 exe/pdb 一起提交。
- `assets/`：Shader、纹理和后续模型资源。

当前构建入口：

```powershell
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Debug -NoPause
```

当前 Debug 运行入口：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Qt.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe --list
```

## 6. 后续 Codex 工作要求

每次开始较大修改前，Codex 应读取：

1. 根目录 `AGENTS.md`
2. `.agents/AGENTS.md`
3. `.agents/CODEX_CONTEXT.md`
4. `DOC/README.md`
5. `DOC/ARCHITECTURE.md`
6. `DOC/CODING_STYLE.md`
7. `DOC/EXTENDING.md`
8. 当前任务涉及模块的 `CMakeLists.txt`、头文件和实现文件

如果本文件与代码事实不一致，以代码和 CMake 当前状态为准，并同步修正文档。
