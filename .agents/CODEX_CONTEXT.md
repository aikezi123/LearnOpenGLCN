# LearnOpenGLCN 当前上下文

本文件是跨对话继续工作的精简快照，不是架构或模块设计正文。正式说明从 `DOC/README.md` 进入；若本文件与代码、CMake 或正式文档不一致，以当前实现为准并同步修正。

## 1. 项目方向

项目使用 C++17、CMake 3.21+、Ninja、MSVC、OpenGL 和 Qt，目标是在保留 LearnOpenGL 教程可运行性的同时，把相机预览、轨迹和可复用技术能力逐步整理为整洁架构。

稳定依赖方向为：外层组合根装配 UI、Infrastructure 和 Application，Application 依赖 Domain，Infrastructure 实现 Application 端口。前期允许在 UI 层保留已跑通的 Qt/OpenGL 原型，但必须明确后续边界。

正式文档入口：

- 总体结构：`DOC/architecture/ARCHITECTURE.md`
- 相机：`DOC/modules/CAMERA_ARCHITECTURE.md`
- 线程池：`DOC/modules/THREAD_POOL.md`
- 二维轨迹：`DOC/modules/TRAJECTORY_2D.md`
- 开发规则：`DOC/guides/CODING_STYLE.md`、`EXTENDING.md`、`TESTING.md`

## 2. 当前实现快照

### 相机与 Qt/OpenGL

- Domain 提供 `ImageFrame` / `PixelFormat`。
- Application 提供同步端口 `ICameraDevice` 与带独立串行控制线程的 `CameraCaptureService`。
- `GalaxyCameraController` 在 Infrastructure 实现端口，并通过 Pimpl 隐藏大恒 SDK。
- `CameraComposition` 创建 Galaxy adapter，将其注入 service，再交给 `CameraImageCaptureView`；UI 不认识厂商 SDK 类型。
- 相机命令使用 FIFO 队列、条件变量和 `promise/future`，关闭时停止接收、排空队列、清理设备并 join。
- SDK 帧通过 Qt queued invoke 切回 UI 线程；最新帧单槽邮箱覆盖过时帧，避免无限堆积界面事件。
- UI 当前会立即对控制命令的 future 调用 `get()`，因此设备操作虽在控制线程执行，界面仍可能短暂等待。
- `DisplayOpenGLImage` 仍是 UI 原型控件，负责现有纹理上传和观察变换，尚未拆成 Infrastructure RAII 资源。

已验证的图片显示路线是 `stb_image + 原生 OpenGL Texture`。此前 `QImage/QOpenGLTexture` 路线曾在关闭窗口后触发 Debug CRT heap corruption；背景及实时纹理更新约定已归档到相机模块文档。

### 通用线程池

- 位于 `infrastructure/concurrency`，target 为 `learnopengl_concurrency` / `learnopengl::concurrency`，只依赖 `Threads::Threads`。
- 当前支持固定 worker、FIFO 队列、条件变量等待、`post()`、通用 `submit()`、future、void/任意返回类型、移动专用 callable/参数、异常传播和排空关闭。
- 八阶段教学路线中，阶段 1–5 按当前范围完成，阶段 6 部分完成，阶段 7–8 留待以后。
- 当前生产代码没有创建或调用线程池，Application 也没有提前定义 `ITaskExecutor`。只有出现真实、可替换的后台执行需求时，才在 Application 定义最小端口并由组合根注入具体实现。
- 相机设备需要串行访问，继续使用专用控制线程，不替换为通用线程池。

### 二维轨迹与课程入口

- `domain/trajectory` 提供二维阿基米德螺旋纯算法；Qt 页面负责后台生成与文本导出。椭圆模型和三维曲面映射尚未实现。
- `LearnOpenGLCN_Lessons.exe` 无参数时打开 Qt 课程导航器，带课程 ID 时运行对应 GLFW 课程；课程仍在独立子进程窗口中运行。
- `lessons` 仍是单一静态库，源码和 include 目录仍通过递归扫描收集。

## 3. 最近验证基线

2026-09-03 的线程池阶段性基线已完成：

```powershell
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Debug -NoPause
ctest --preset ninja-msvc-debug --output-on-failure
```

当次 Debug 全量配置和构建成功，CTest 20/20 通过。此结论只说明当时基线；后续代码或 CMake 变化后必须重新验证。

常用运行入口：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Qt.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe --list
```

## 4. 后续边界

- 相机：优先补 fake device 行为测试，再考虑 UI 非阻塞观察 future；最后单独评估 `DisplayOpenGLImage` 的资源拆分。
- 线程池：若恢复八阶段路线，从阶段 6 的完整 shutdown 语义继续，再做健壮性和性能能力。
- 轨迹：按真实需求增加椭圆参数化和三维曲面映射，不提前混入当前二维模型。
- 课程：Qt 导航器与 GLFW 课程窗口暂时分离；模块化 lessons 构建应作为独立重构任务。

开始新任务时仍需检查 `git status --short`，读取相关正式文档、CMake 和源码，不以本快照替代现场核对。
