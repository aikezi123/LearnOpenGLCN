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

当前已建立相机采集的最小分层切片：`domain` 提供 `ImageFrame` / `PixelFormat`，`application` 提供 `ICameraDevice` 和 `CameraCaptureService`，`infrastructure/camera/galaxy` 提供 `GalaxyCameraController` 大恒适配器。大恒 SDK 头文件只出现在 infrastructure 的 `.cpp` 中，不再暴露给 UI 或 application。`CameraCaptureService` 已使用纯 C++17 独立控制线程、FIFO 命令队列、条件变量和 `Closed / Opened / Captured` 状态串行访问设备，当前 `Captured` 的语义是“正在采集”；命令由一个 `std::function<CameraResult(ICameraDevice&)>` 与 `std::promise<CameraResult>` 组成，公开请求返回 `std::future<CameraResult>`。已接受、拒绝、非法状态和设备异常路径都会完成 future；`shutdown()` 采用停止入队、排空队列、注销回调、停止采集、关闭设备和同线程销毁设备的顺序。

`CameraCaptureService` 当前公开打开第一个设备、按 ID/名称打开、开始/停止采集、关闭、曝光时间、增益、帧率、自动白平衡和帧回调设置请求。只有控制线程写相机状态，外部通过原子状态读取最近一次已完成操作的快照。Qt UI 只在现有相机页面边界调用这些纯 C++ 接口，并在最终图像帧进入 `DisplayOpenGLImage` 前使用 Qt queued invoke 切回 UI 线程。控制请求当前在 UI 中立即调用 `future.get()`，所以设备操作在线程外执行但 UI 同步等待；这是当前阶段保留的简化。

当前学习版本保留用户原有的 `CameraCaptureService` 代码骨架：成员线程仍为 `m_thread`，命令继续由 `queue + condition_variable + promise/future` 处理，没有增加关闭互斥锁或新的命令包装层。`CameraImageCaptureView` 创建后仍自动尝试打开第一台设备并开始采集，同时提供按 ID/名称打开、开始/停止/关闭，以及自动白平衡、曝光时间、增益和 FPS 参数控件；无相机时只显示错误状态，不阻止程序启动。

当前 Qt 主窗口为左侧 `QTreeWidget` 导航 + 右侧 `QStackedWidget` 页面容器。`composition_root/AppComposition` 组织应用页面，`modules/CameraComposition` 创建 `GalaxyCameraController`，将其作为 `ICameraDevice` 注入 `CameraCaptureService`，再创建 `CameraImageCaptureView`；`modules/TrajectoryComposition` 创建轨迹页面。`qt_main.cpp` 只负责 Qt/OpenGL 初始化、显示窗口和进入事件循环。相机窗口通过 Qt queued invoke 把 SDK 线程中的 `domain::ImageFrame` 投递到 UI 线程，再调用提升控件 `DisplayOpenGLImage::setRgb24Frame(...)`。UI 使用互斥锁保护的单槽最新帧邮箱：已有显示任务排队时只覆盖旧帧，不逐帧追加 Qt 事件。UI 不直接处理相机 SDK 类型。

相机控制线程、命令队列、条件变量、锁、原子状态、promise/future、shutdown 和最新帧投递机制已经集中记录在 `DOC/CAMERA_ARCHITECTURE.md`。当前相机控制阶段到此结束；本阶段明确不拆分 `DisplayOpenGLImage`，它继续作为 UI 原型控件承担现有 OpenGL 显示职责。

分层代码目录统一采用“层 / 模块 / include + src”的模块优先结构。当前使用 `application/camera/include/camera/...`、`domain/image/include/imageframe/...`、`infrastructure/camera/galaxy/include/camera/galaxy/...`、`infrastructure/shader/include/shader/...`。公共头文件目录保持简洁，不重复嵌套项目名和当前层名。

application 层流程对象统一使用 `Service` 命名，例如 `CameraCaptureService`。端口接口和使用它的 service 放在 application，例如 `ICameraDevice`；它表达 application 为完成流程需要外部系统提供的能力，由 infrastructure 实现。domain 层只放稳定概念和值对象，例如 `ImageFrame`、`PixelFormat`，不放需要驱动外部系统干活的接口。

术语约定：`GalaxyCameraController : ICameraDevice` 这种继承重写叫实现端口；`CameraComposition` 创建 `GalaxyCameraController`，把它作为 `ICameraDevice` 传给 `CameraCaptureService`，这个从外部传入依赖的动作叫依赖注入。依赖注入不是消除依赖，而是让 service 依赖抽象端口，不直接依赖具体实现。

依赖倒置（DIP）与依赖注入（DI）需要区分：DIP 解决编译依赖方向，Application 定义并依赖 `ICameraDevice`，Infrastructure 反过来依赖 Application 并由 `GalaxyCameraController` 实现该接口；DI 解决对象创建和传递，`CameraComposition` 创建具体适配器并通过构造函数传给 `CameraCaptureService`。接口本身不是保存对象的插槽，真正保存 `unique_ptr<ICameraDevice>` 的是 service。两者结合后，Application 不包含 Infrastructure 具体类型，运行时仍通过虚函数动态分派到 Galaxy 实现。

组合根与 UI/Infrastructure 的边界已经明确：UI 只依赖 Application，不直接包含或创建 Infrastructure 具体类；Infrastructure 实现 Application 端口；组合根位于最外层，可以同时知道 UI、Application 和 Infrastructure，并负责选择具体实现、创建对象和转移所有权。编译依赖与运行调用必须分开理解：代码上 `CameraImageCaptureView` 不认识 `GalaxyCameraController`，运行时调用仍会沿 `CameraImageCaptureView -> CameraCaptureService -> ICameraDevice -> GalaxyCameraController` 执行。

`MainWindow` 仍通过 `QStackedWidget` 包含并显示多个页面，但不再创建具体页面及其后台依赖。`AppComposition` 汇总模块并向 `MainWindow` 注册通用 `QWidget`；`CameraComposition` 和 `TrajectoryComposition` 分别创建各自对象图。Qt 父子机制拥有页面，`CameraImageCaptureView` 通过 `unique_ptr` 拥有 `CameraCaptureService`，service 再通过 `unique_ptr<ICameraDevice>` 拥有实际相机适配器。组合根负责装配，不要求长期持有全部对象。

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
- `domain/`、`application/`：已形成相机预览所需的最小 target，当前包含图像帧模型、二维轨迹纯算法、相机端口和预览 service。
- `third_party/`：GLAD、GLFW、GLM、stb_image 等第三方依赖；GLFW 3.4 的 Windows x64 VC2022 静态库与许可证保存在仓库中，课程构建不依赖本机安装 GLFW。
- `third_party/Galaxy/`：大恒 Galaxy SDK 的第三方依赖包装目录，当前 CMake target 为 `Galaxy::SDK`。仓库保存同一版本的 VC/C API 与 C++ SDK 头文件、当前适配器实际链接的 `GxIAPICPPEx.lib`，以及 `GxIAPICPPEx.dll` 的 13 个 Win64 递归运行依赖；CMake 自动部署这些 DLL，不依赖本机 SDK 安装路径。
- `assets/`：Shader、纹理和后续模型资源。

当前 `LearnOpenGLCN_Lessons.exe` 不带参数会打开 Qt 课程导航器。课程清单来自 `lessons/catalog` 的 `LessonRegistry`，左侧按 LearnOpenGL 入门章节顺序列出已实现课程，右侧显示课程信息、运行按钮和子进程输出。点击运行时，导航器启动同一个 exe 并传入课程 ID，实际 GLFW 课程画面仍在独立窗口中运行；传入课程 ID 时仍可直接运行，例如 `LearnOpenGLCN_Lessons.exe transform`。导航窗口实现已拆到 `composition_root/lesson_launcher`，`lesson_main.cpp` 只保留入口与命令行选择逻辑。

当前构建入口：

```powershell
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Debug -NoPause
```

项目提供独立的 `ninja-msvc-asan` CMake preset，用于在 VS Code + CMake Tools 中构建和调试 MSVC AddressSanitizer 版本。ASan 构建目录为 `out/build/ninja-msvc-asan`，不会改变普通 Debug/Release 配置；构建后自动把 x64 Debug ASan 运行时 DLL 部署到 `bin`。

当前 Debug 运行入口：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Qt.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe --list
```

## 6. 当前阶段完成总结

当前阶段已经完成相机预览的最小整洁架构切片和课程入口整理：

- `domain` 保存稳定图像帧概念：`ImageFrame` / `PixelFormat`。
- `domain/trajectory` 保存二维轨迹纯算法：`ArchimedeanSpiral2DGenerator` 生成固定阿基米德螺旋上的 XOY 平面采样点，线间距全局固定，不处理 Z 方向、文件导出或 OpenGL 绘制。
- `application` 保存相机采集流程和端口：`ICameraDevice` / `CameraCaptureService`。端口放 application，不放 domain。
- `infrastructure/camera/galaxy` 保存大恒相机适配：`GalaxyCameraController` 实现 `ICameraDevice`，并用 Pimpl 隐藏大恒 SDK 头文件、SDK 成员和回调类。
- `composition_root/modules/CameraComposition` 负责相机依赖装配：大恒相机实现 -> `ICameraDevice` -> `CameraCaptureService` -> Qt UI；`AppComposition` 负责汇总各模块页面，`qt_main.cpp` 只保留启动流程。
- Qt 主窗口使用左侧 `QTreeWidget` 导航和右侧 `QStackedWidget` 页面栈；相机页面为独立 `CameraImageCaptureView`。
- `DisplayOpenGLImage` 当前仍是 UI 原型控件，负责相机帧纹理上传和显示变换。它支持水平/垂直翻转、左右 90 度旋转、缩放、平移、重置和矩形/圆形显示切换；圆形控件外观只通过 QWidget mask 完成，圆形模式的纹理坐标中心裁取与正方形 viewport 保证观察变换作用于 1:1 图像。
- `LearnOpenGLCN_Lessons.exe` 已有课程导航器和 `lessons/catalog` 注册表。无参数打开 Qt 导航器，带课程 ID 直接运行 GLFW 课程，`lesson_main.cpp` 只保留入口与命令行选择逻辑，导航窗口实现位于 `composition_root/lesson_launcher`。

本阶段还完成了二维阿基米德螺旋轨迹的最小闭环：

- `domain/trajectory/ArchimedeanSpiral2DGenerator` 固定实现圆形阿基米德螺旋 `r = A + B*theta`，只生成 XOY 点；线间距全局固定，半径分段仅改变目标点间距。
- 点距求解先使用局部弧长微分估算 `dtheta`，再对“实际二维点距 - 目标点距”的距离残差进行二分求解；残差命名统一使用 `residual`，避免和程序错误混淆。
- `generate(ranges)` 合并保存所有分段的点到 `m_points`，点内以 `rangeIndex` 记录所属分段；共享边界点去重，启用 `appendRangeEndPoint` 时允许补一个精确边界点，因此最后剩余距离可能小于目标点距。
- Qt 主窗口新增“轨迹算法 / 螺旋线导出”页面。它在后台生成每段并同时输出总文件、分段坐标文件和分段迭代次数文件；页面显示生成、写入和总耗时，关闭时安全等待后台任务结束。
- 独立 C++ 导出工具位于 `tools/trajectory`，是单独 CMake 项目，不由主工程顶层 CMake 构建。仓库内不保留 MATLAB 辅助脚本。
- 详细说明统一记录在 `DOC/TRAJECTORY_2D.md`。椭圆模型和三维曲面映射尚未实现；后续应替换椭圆的 `pointAt()`/局部速度公式，并按曲面模型计算 Z。

仍需注意的阶段性债务：

- `DisplayOpenGLImage` 中的 Shader、VAO/VBO/EBO、Texture 尚未迁移为 infrastructure RAII 资源。
- GLFW 课程仍以独立窗口运行，导航器通过子进程启动课程，没有嵌入 Qt 右侧面板。
- lessons 仍是单一静态库，课程源码和 include 目录仍通过递归扫描收集。

## 7. 后续 Codex 工作要求

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
