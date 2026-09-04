# EngineeringLab 当前上下文

本文件是跨对话继续工作的精简快照。正式说明从 `DOC/README.md` 进入；若本文件与代码、CMake 或正式文档不一致，以当前实现为准并同步修正。

## 1. 项目定位

EngineeringLab 是个人 C++ 工程技术持续学习与实验平台。图形学、工业相机、轨迹算法、CAD、机器人、CUDA 和 IPC 等属于可交叉组合的学习方向，不直接等同于顶层代码模块。

工程继续使用 domain、application、infrastructure、ui 和 composition_root 分层。目录表达职责，CMake target 表达真实技术依赖；不为未来方向提前创建空目录、端口或包装层。

## 2. 当前命名与构建边界

- C++ 根命名空间：`engineeringlab`
- CMake alias 前缀：`englab::`
- 综合工作台：`EngineeringWorkbench`
- OpenGL 课程入口：`OpenGLLessons`
- 核心 target：`englab::domain`、`englab::application`、`englab::ui`
- 技术 target：`englab::graphics_opengl`、`englab::camera_galaxy`、`englab::concurrency`
- 课程 target：`englab::opengl_lessons`

OpenGL 课程只链接实际使用的 OpenGL、GLFW、GLAD、GLM 和 stb_image 能力，不再通过完整 infrastructure target 间接链接 Galaxy 相机或线程池。

## 3. 当前实现快照

- Domain 提供 `ImageFrame` / `PixelFormat` 和二维阿基米德螺旋算法。
- Application 提供 `ICameraDevice` 与带独立串行控制线程的 `CameraCaptureService`。
- `GalaxyCameraController` 实现相机端口，并通过 Pimpl 隐藏大恒 SDK。
- Qt 相机页面通过最新帧单槽邮箱切回 UI 线程；`DisplayOpenGLImage` 仍负责当前 OpenGL 纹理和观察变换。
- `englab::concurrency` 提供固定线程池，当前生产代码尚未使用，直接消费者只有测试。
- `OpenGLLessons` 无参数时打开 Qt 课程导航器，带课程 ID 时运行对应 GLFW 课程。

## 4. 构建与验证

```powershell
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Debug -NoPause
ctest --preset ninja-msvc-debug --output-on-failure
```

运行入口：

```powershell
.\out\build\ninja-msvc-debug\bin\EngineeringWorkbench.exe
.\out\build\ninja-msvc-debug\bin\OpenGLLessons.exe
.\out\build\ninja-msvc-debug\bin\OpenGLLessons.exe --list
```

2026-09-04 已使用 `ninja-msvc-debug` 完成重新配置和全量构建；CTest 实际发现并通过 20/20 个用例。`OpenGLLessons.exe --list` 运行成功。`EngineeringWorkbench.exe` 已完成 5 秒启动冒烟检查；自动 CloseMainWindow 未使其退出，验证脚本随后终止了进程，因此正常交互关闭流程仍未验证。