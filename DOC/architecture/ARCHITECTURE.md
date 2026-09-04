# 代码架构与依赖关系

## 0. 长期目标与阶段策略

EngineeringLab 是个人 C++ 工程技术持续学习与实验平台。LearnOpenGL 课程是当前学习主线之一，但不再代表整个工程的范围。项目可以逐步纳入图形学、工业相机、轨迹与几何算法、CAD、机器人、CUDA、IPC 和其他工程主题。

这些主题是可以交叉组合的学习方向，不直接映射为互相独立的顶层模块。代码首先按照职责进入 domain、application、infrastructure、ui、composition_root、lessons 或 tools；只有真实的依赖隔离、复用或构建需求出现时，才拆出更细的 CMake target。

课程代码应保持可运行、可对照教程、可逐步实验。准备长期复用的模型、流程和技术实现再按稳定边界进入正式分层。前期允许在 `ui` 层完成 Qt/OpenGL 原型，但文档必须区分当前原型和目标边界。

## 1. 架构目标

工程计划采用整洁架构。核心目标不是增加目录数量，而是让依赖始终指向更稳定的内层，使领域和应用逻辑不依赖 GLFW、OpenGL、文件系统等外部技术细节。

需要长期遵守的依赖规则是：

> 内层模块不能包含或链接外层模块；外层通过实现内层定义的接口完成能力注入。

目标依赖关系如下：

```text
                         ┌──────────────────┐
                         │ app              │
                         │ Composition Root │
                         └───────┬──────────┘
                                 │ 创建和装配
              ┌──────────────────┼──────────────────┐
              ▼                  ▼                  ▼
       ┌────────────┐   ┌────────────────┐   ┌────────────┐
       │ ui         │   │ infrastructure │   │ lessons    │
       │ Qt/GLFW UI │   │ OpenGL/stb/FS  │   │ 教程/功能  │
       └──────┬─────┘   └───────┬────────┘   └─────┬──────┘
              │                 │                  │
              └─────────────────┼──────────────────┘
                                ▼
                       ┌─────────────────┐
                       │ application     │
                       │ 用例与端口接口  │
                       └────────┬────────┘
                                ▼
                       ┌─────────────────┐
                       │ domain          │
                       │ 纯模型与规则    │
                       └─────────────────┘
```

`third_party/`、操作系统 OpenGL 库和相机 SDK 只允许被外层适配器使用，不能向 `application` 或 `domain` 泄漏。

## 2. 当前实现

### 2.1 构建结构

顶层 `CMakeLists.txt` 依次加载第三方依赖、domain、application、infrastructure、lessons、ui、composition_root 和 tests。目录表达代码职责，CMake target 表达真实的编译与第三方依赖边界。

当前主要 target：

| Target | 类型 | 主要职责 | 直接依赖 |
| --- | --- | --- | --- |
| `EngineeringWorkbench` | Executable | Qt 综合学习工作台与对象装配 | `englab::ui`、`englab::application`、`englab::camera_galaxy`、Qt6 Widgets |
| `OpenGLLessons` | Executable | LearnOpenGL 课程导航器与课程入口 | `englab::opengl_lessons`、Qt6 Widgets |
| `engineeringlab_opengl_lessons` | Static | 收集并编译当前 OpenGL 课程 | `englab::graphics_opengl`、GLAD、GLFW、OpenGL、GLM、stb_image |
| `engineeringlab_ui` | Static | Qt/OpenGL 显示原型、相机与轨迹页面 | `englab::application`、`englab::domain`、Qt/OpenGL UI 依赖 |
| `engineeringlab_graphics_opengl` | Static | 当前课程使用的 OpenGL Shader 技术能力 | GLAD、OpenGL |
| `engineeringlab_camera_galaxy` | Static | 大恒相机适配器 | `englab::application`、`Galaxy::SDK` |
| `engineeringlab_concurrency` | Static | 不依赖 GUI/OpenGL 的线程池 | `Threads::Threads` |
| `engineeringlab_application` | Static | 相机预览 service 与相机端口 | `englab::domain`、`Threads::Threads` |
| `engineeringlab_domain` | Static | 图像帧模型和二维轨迹纯算法 | 无项目内依赖 |

`tests/` 位于生产模块之外，只允许单向依赖被测 target。测试 target 使用 `engineeringlab_` 前缀，并通过 `engineeringlab_add_gtest()` 注册。

OpenGL、Galaxy 相机和线程池不再被聚合进单一 `infrastructure` 静态库。拆分的目的只是隔离技术依赖，不表示图形学、视觉、CAD 或机器人被定义为互相独立的业务模块。

#### 2.1.1 线程池并发能力边界

`ThreadPool` 位于 `infrastructure/concurrency`，是固定工作线程和共享 FIFO 队列的具体技术实现。它形成独立 `engineeringlab_concurrency` 静态库，仅依赖 `Threads::Threads`，不依赖 GUI、OpenGL、相机 SDK、application 或 domain。当前生产代码尚未创建或调用线程池；直接消费者只有测试 target。线程池保持独立，不由聚合 infrastructure target 向其他消费者传播。

application 当前没有 `ITaskExecutor`。只有实际 application service 出现后台执行需求时，才根据用例定义最小执行器端口；线程池适配器或其他执行策略在外层实现该端口，组合根负责选择具体实现、创建实例并注入。业务模块不继承具体 `ThreadPool`，相机的串行设备控制线程也继续保持专用执行模型。

当前线程池提供 `post()`、通用 `submit()`和排空式 `shutdown()`。本轮基线完成八阶段路线中的阶段1–5；阶段6只完成外部并发/重复关闭、排空与关闭后拒绝等当前范围，阶段7–8留待后续。详细行为和已知边界见[线程池并发模块](../modules/THREAD_POOL.md)。

`third_party/Galaxy` 当前保存大恒 VC/C API、C++ SDK 头文件和 MSVC x64 import library。大恒运行时 DLL 不再由 CMake 查找或复制，而是直接放在 `out/build/<preset>/bin`，随 exe、pdb 等运行产物一起提交。

`composition_root/qt_main.cpp` 只负责初始化 Qt/OpenGL、调用 `AppComposition` 创建窗口并进入事件循环。`AppComposition` 负责组织模块页面，`modules/CameraComposition` 和 `modules/TrajectoryComposition` 分别装配相机与轨迹功能。`composition_root/lesson_main.cpp` 负责解析命令行参数、直接运行课程或启动 LearnOpenGL 课程导航器；导航窗口实现放在 `composition_root/lesson_launcher`。课程清单集中在 `lessons/catalog` 的 `LessonRegistry` 中。所有课程源码仍被编入同一个 `engineeringlab_opengl_lessons` 静态库，因此即使某课程没有运行，它仍必须成功编译。

当前已经拆成两个 executable：`OpenGLLessons` 和 `EngineeringWorkbench`。教程入口和 Qt 工程入口不再共享同一个 `main.cpp`，避免手动改入口来切换运行内容。

`ui/` 目前包含 Qt Widgets 与 `QOpenGLWidget` 原型。`MainWindow` 提供左侧 `QTreeWidget` 导航和右侧 `QStackedWidget` 页面容器，并接收装配层创建好的页面，不再创建具体功能实现。相机 SDK 控制已经从 UI 拆到 `infrastructure/camera/galaxy`，通过 `application` 中的 `ICameraDevice` 和 `CameraCaptureService` 注入到相机页面。相机页面的翻转、旋转、缩放、平移按钮属于 UI 交互，具体显示变换暂时由 `DisplayOpenGLImage` 通过 shader uniform 矩阵完成。纹理加载、Shader 编译和绘制流程仍暂时集中在 UI 类中，以便继续稳定 Qt OpenGL 显示路径；后续应再拆出 OpenGL 资源和图像加载能力，避免长期把渲染细节堆在 UI 层。

### 2.2 当前运行流程

当前 `EngineeringWorkbench` 的运行流程是 Qt 原型入口：

```text
main()
  ├── 设置 QSurfaceFormat，要求 OpenGL 3.3 Core Profile
  ├── 创建 QApplication
  ├── AppComposition 创建 MainWindow
  │   ├── CameraComposition 装配 GalaxyCameraController
  │   │   → ICameraDevice → CameraCaptureService → CameraImageCaptureView
  │   ├── TrajectoryComposition 创建 TrajectoryExportView
  │   └── 将两个页面注册到 MainWindow
  ├── 显示主窗口
  └── 进入 Qt 事件循环 app.exec()
```

当前 `OpenGLLessons` 未传参数时打开 Qt 课程导航器；左侧按 LearnOpenGL 入门章节顺序列出已实现课程，右侧显示课程信息、运行按钮和子进程输出。点击运行时，导航器用子进程启动同一个 exe 并传入课程 ID；带课程 ID 参数时仍直接运行对应课程。以坐标变换课程为例，典型 GLFW 课程流程是：

```text
main()
  └── transform()
      ├── 初始化 GLFW
      ├── 创建窗口和 OpenGL 上下文
      ├── 初始化 GLAD
      ├── 创建 VAO/VBO/EBO
      ├── 通过 stb_image 加载纹理
      ├── 通过 Shader 加载 GLSL
      ├── 循环处理输入、计算矩阵并绘制
      └── 释放资源并终止 GLFW
```

这是一种适合教学的完整示例结构，但窗口、渲染循环和 GPU 资源管理会在多个课程中重复。

Qt 导航器和 GLFW 课程使用不同的事件循环和窗口/context 管理方式。因此当前导航器不把 GLFW 画面强行嵌入右侧面板，而是通过子进程隔离运行课程。若未来需要真正在右侧面板内显示课程画面，应逐课迁移为 `QOpenGLWidget` 或抽出共享渲染接口。

### 2.3 当前入口拆分

当前 `composition_root/` 中有两个独立入口：

| Executable | 职责 | 建议依赖 | 说明 |
| --- | --- | --- | --- |
| `OpenGLLessons` | 显示课程导航器或直接运行 LearnOpenGL 教程代码 | `englab::opengl_lessons`、Qt6 Widgets | 面向课程学习；导航器使用 Qt，课程仍允许使用 GLFW 教学式完整流程。 |
| `EngineeringWorkbench` | 运行 Qt/OpenGL 工程原型 | `engineeringlab_ui` | 面向相机图像、点云、轨迹等 Qt 显示模块。 |

拆分后不再需要为了切换运行内容频繁修改同一个 `main.cpp`。两个入口都属于 `composition_root` 层，并保持很薄：

- lesson 入口只负责显示课程导航、通过课程名选择课程、调用课程函数或启动课程子进程、返回进程结果。
- Qt 入口只负责设置 Qt/OpenGL 格式、创建 `QApplication`、显示顶层窗口、进入事件循环。
- 两个入口都不能放 Shader 编译、纹理加载、相机采集、点云处理等业务或渲染细节。

当前 lesson 入口已支持 Qt 导航和命令行选择：

```powershell
OpenGLLessons.exe
OpenGLLessons.exe transform
OpenGLLessons.exe --list
```

不带参数会打开 Qt 课程导航器；传入课程 ID 会直接运行课程；`--list` 只打印课程清单。

## 3. 目标分层职责与边界

### 3.1 domain

职责：

- 表达稳定的领域数据和规则。
- 保存与具体图形 API 无关的模型，例如颜色、变换、网格描述和场景数据。
- 执行不需要系统资源的纯计算。
- 表达“是什么”的概念，例如 `ImageFrame`、`PixelFormat`、相机设备描述、相机参数值对象等。

当前 `domain/trajectory` 包含 `ArchimedeanSpiral2DGenerator`，用于生成固定阿基米德螺旋 `r = A + Bθ` 上的二维采样点。该算法只使用标准库，只控制 XOY 平面点距；线间距全局固定并用于计算 `B`，不同半径段只改变目标点间距。详见 [二维阿基米德螺旋轨迹](../modules/TRAJECTORY_2D.md)。

允许依赖：

- C++ 标准库。
- `domain` 内部其他模块。

禁止依赖：

- OpenGL、GLAD、GLFW、stb_image。
- 文件路径、窗口句柄和 GPU 对象 ID。
- `application`、`infrastructure`、`ui`、`lessons`、`app`。

领域模型不足时允许该层保持精简，不为满足目录形式而制造无意义抽象。

不应把需要驱动外部系统干活的接口放到 domain。比如 `ICameraDevice` 包含打开相机、开始采集、停止采集、注册帧回调等动作，它是相机预览流程需要的外部能力端口，属于 application，而不是 domain。

### 3.2 application

职责：

- 编排用例和程序流程；本项目应用层流程对象统一采用 `Service` 命名，例如 `CameraCaptureService`。
- 定义外部能力端口，例如 `ICameraDevice`、`IRenderer`、`IImageLoader`、`IClock`。
- 使用 `domain` 模型表达输入和输出。
- 定义与具体 OpenGL 实现无关的课程运行协议。

端口接口应和使用它的 application service 放在同一层。application 通过端口描述“为了完成这个流程，需要外部世界提供什么能力”；具体实现由 infrastructure 提供。比如 `CameraCaptureService` 使用 `ICameraDevice`，而大恒的 `GalaxyCameraController` 在 infrastructure 中实现 `ICameraDevice`。

允许依赖：

- `domain`。
- C++ 标准库。

禁止依赖：

- OpenGL、GLAD、GLFW 和 stb_image 的头文件或类型。
- `infrastructure`、`ui` 和 `app` 的具体类。

### 3.3 infrastructure

职责：

- 实现 application 定义的技术端口。
- 封装 OpenGL Shader、Buffer、VertexArray、Texture 等资源。
- 实现基于 stb_image 的图片加载器。
- 处理资源路径、文件读取和技术错误转换。
- 对相机 SDK、平台 API 等重型第三方适配器，优先使用 Pimpl 将 SDK 类型和回调类隐藏在 `.cpp` 中，公共头文件只暴露项目接口。

边界要求：

- OpenGL 对象必须有清晰的所有权，优先使用 RAII。
- 具体实现可以依赖 application/domain，反向依赖不允许出现。
- 第三方类型尽量停留在 `.cpp` 或 infrastructure 的私有头文件中。
- 如果公开类必须持有第三方 SDK 对象，优先改为公开类持有 `std::unique_ptr<Impl>`，由 `Impl` 在 `.cpp` 中包含和使用第三方头文件。
- 不在该层决定具体运行哪一课程。

### 3.4 ui

职责：

- 创建和管理窗口。
- 读取键盘、鼠标及窗口事件。
- 将平台事件转换为 application 能理解的输入。
- 后续可承载课程选择界面或调试 UI。
- 当前 Qt 原型阶段可使用 `QApplication`、`QWidget`、`QOpenGLWidget`、Qt Designer `.ui` 文件和 Qt 事件循环。

边界要求：

- Qt/GLFW 类型不得进入 domain 或 application。
- UI 不直接实现领域规则。
- UI 触发 application 用例，而不是反向控制 application 内部状态。
- 前期为了跑通功能允许在 UI 层临时实现完整渲染流程，但该代码应被视为原型；当流程稳定、重复或需要复用时，拆分到 application/infrastructure。

### 3.5 lessons

职责：

- 组织 LearnOpenGL 教程章节和练习。
- 展示一个完整且可运行的图形知识点。
- 作为功能模块注册到程序入口或课程目录中。

课程有两种合理形态：

1. 教学型课程直接操作 OpenGL。此时它属于外层，可以依赖 OpenGL infrastructure。
2. 抽象型课程只调用 application 端口。此时它可以被其他渲染实现复用。

不应为了架构形式隐藏所有 OpenGL 调用，否则会削弱教程代码的教学价值。

### 3.6 app

职责：

- 作为唯一的 Composition Root 创建具体对象。
- 把 infrastructure/ui 实现注入 application 或 lesson。
- 解析启动参数并选择课程。
- 控制进程级启动和退出。

`app` 可以知道所有外层具体类型，但其他层不得依赖 `app`。

当前入口目录名为 `composition_root/`，承担 app/Composition Root 角色。`qt_main.cpp` 保持为薄入口，应用级组织放在 `AppComposition`，各功能对象图放在 `modules/*Composition`。不要在 `main.cpp` 中堆积业务逻辑、Shader 编译、纹理加载或相机采集流程。

术语约定：

- `application` 定义端口，例如 `ICameraDevice`。
- `infrastructure` 继承并重写端口，例如 `GalaxyCameraController : ICameraDevice`，这叫“实现端口”。
- `composition_root` 创建具体实现，并把它按端口类型传给需要该依赖的 service，例如 `CameraComposition` 把 `GalaxyCameraController` 作为 `ICameraDevice` 传给 `CameraCaptureService`，这个“从外部传入依赖”的动作叫“依赖注入”。

依赖注入不是消除依赖，而是让 service 依赖抽象端口，把“使用哪个具体实现”的决定权移动到组合根。

#### 3.6.1 UI、Infrastructure 与组合根的关系

UI 不应直接包含或创建 Infrastructure 的具体实现。例如，`CameraImageCaptureView` 不能直接 `#include <camera/galaxy/GalaxyCameraController.h>` 并创建大恒相机，否则 UI 会和 Galaxy SDK 适配器绑定，后续切换海康相机、模拟相机或测试实现时都必须修改 UI。

组合根位于最外层，可以同时依赖 UI、Application 和 Infrastructure，因此由它完成具体实现的选择与连接。当前相机模块的编译依赖关系为：

```text
ui -----------------> application
infrastructure -----> application
composition_root ---> ui
composition_root ---> infrastructure
composition_root ---> application
```

UI 和 Infrastructure 在代码上互相不知道对方，但在运行时仍然通过 Application 端口协作：

```text
CameraImageCaptureView
    -> CameraCaptureService
    -> ICameraDevice 虚函数
    -> GalaxyCameraController
    -> Galaxy SDK
```

必须区分这两个方向：

- **编译依赖**描述某层需要包含和链接哪些类型。UI 只认识 `CameraCaptureService`，不认识 `GalaxyCameraController`。
- **运行调用**描述程序运行时请求最终由哪个对象执行。UI 发起的相机操作仍会通过 `ICameraDevice` 动态分派到 `GalaxyCameraController`。

因此，组合根不只是为了绕开一条 include 限制；它集中决定“本次程序使用哪个具体实现”、创建多少个实例、哪个页面使用哪个 service，以及对象所有权如何转移。

#### 3.6.2 依赖倒置与依赖注入

依赖倒置（Dependency Inversion Principle，DIP）和依赖注入（Dependency Injection，DI）经常一起使用，但解决的是两个不同问题：

| 概念 | 解决的问题 | 当前相机模块中的体现 |
| --- | --- | --- |
| 依赖倒置 | 编译依赖应该指向谁 | Application 定义 `ICameraDevice`，Infrastructure 实现它 |
| 依赖注入 | 具体对象由谁创建，怎样交给使用者 | `CameraComposition` 创建 `GalaxyCameraController` 并传给 `CameraCaptureService` |

如果 `CameraCaptureService` 在内部直接创建大恒相机：

```cpp
class CameraCaptureService {
private:
    std::unique_ptr<GalaxyCameraController> m_cameraDevice;
};
```

Application 就必须包含 Infrastructure 的具体类型，编译依赖方向变成：

```text
application
    -> infrastructure
    -> Galaxy SDK
```

依赖倒置要求高层策略不要依赖低层技术细节，而是由高层所在的 Application 定义自己需要的抽象契约：

```cpp
class ICameraDevice {
public:
    virtual ~ICameraDevice() = default;
    virtual CameraResult startCapture() = 0;
    virtual CameraResult stopCapture() = 0;
};
```

`CameraCaptureService` 只依赖这个契约：

```cpp
class CameraCaptureService {
public:
    explicit CameraCaptureService(
        std::unique_ptr<ICameraDevice> cameraDevice
    );

private:
    std::unique_ptr<ICameraDevice> m_cameraDevice;
};
```

Infrastructure 反过来依赖并实现 Application 定义的接口：

```cpp
class GalaxyCameraController final
    : public application::ICameraDevice {
};
```

编译依赖因此被倒置为：

```text
infrastructure
    -> application::ICameraDevice
```

Application 不再知道 Galaxy、海康或模拟相机的具体类型。这里的“倒置”是编译依赖方向发生了变化，并不表示运行时调用方向也反转。

依赖注入负责把具体实现交给使用者。当前由组合根执行构造函数注入：

```cpp
auto cameraDevice =
    std::make_unique<GalaxyCameraController>();

auto cameraService =
    std::make_unique<CameraCaptureService>(
        std::move(cameraDevice)
    );
```

这里不是把对象存进“接口本身”，而是把 `GalaxyCameraController` 以 `ICameraDevice` 接口类型传给 `CameraCaptureService`，由 service 保存并使用：

```text
unique_ptr<GalaxyCameraController>
    -> unique_ptr<ICameraDevice>
    -> CameraCaptureService::m_cameraDevice
```

最终形成的职责分工是：

```text
application
    ├── 定义 ICameraDevice
    └── CameraCaptureService 依赖 ICameraDevice

infrastructure
    └── GalaxyCameraController 实现 ICameraDevice

composition_root
    ├── 创建 GalaxyCameraController
    └── 注入 CameraCaptureService
```

依赖倒置和依赖注入应组合理解：

- DIP 让 Application 依赖自己拥有的抽象，而不依赖 Infrastructure 具体实现。
- DI 让 Application 不必在内部创建具体实现，由最外层组合根选择并传入。
- 只把对象作为参数传入属于 DI；只有同时让内层依赖抽象、外层实现抽象，才形成当前架构需要的依赖方向。

#### 3.6.3 页面包含关系与对象创建关系

引入组合根以后，`MainWindow` 仍然在界面结构上包含多个子页面，但不再负责创建具体页面及其后台依赖。两种关系需要分开理解：

- **界面包含关系**：`MainWindow` 的 `QStackedWidget` 保存并显示多个 `QWidget` 页面。
- **对象创建关系**：`CameraComposition`、`TrajectoryComposition` 等模块装配类创建具体页面和后台对象，`AppComposition` 将页面注册到 `MainWindow`。

`MainWindow` 只通过通用接口接收页面：

```cpp
void addBusinessPage(
    const QString& categoryName,
    const QString& pageName,
    QWidget* page
);
```

因此它不需要包含 `CameraImageCaptureView`、`TrajectoryExportView` 或 `GalaxyCameraController`。新增页面通常只修改对应模块装配类和 `AppComposition`，不会继续扩大 `MainWindow` 的具体依赖。

当前相机对象链的创建和所有权为：

```text
CameraComposition 创建对象并转移所有权
    |
    +-> CameraImageCaptureView
            |
            +-> unique_ptr<CameraCaptureService>
                    |
                    +-> unique_ptr<ICameraDevice>
                            |
                            +-> 实际对象 GalaxyCameraController
```

页面加入 `QStackedWidget` 后由 Qt 父子对象机制管理；页面通过 `std::unique_ptr` 拥有 service，service 再通过 `std::unique_ptr<ICameraDevice>` 拥有具体相机适配器。组合根负责创建、选择和连接对象，不要求自己长期持有所有对象。

当模块数量增加时，组合根也不应退化成一个巨大的 `qt_main.cpp`。当前采用以下拆分：

```text
composition_root/
├── qt_main.cpp
├── AppComposition.h
├── AppComposition.cpp
└── modules/
    ├── CameraComposition.h
    ├── CameraComposition.cpp
    ├── TrajectoryComposition.h
    └── TrajectoryComposition.cpp
```

`qt_main.cpp` 只保留进程启动；`AppComposition` 汇总应用模块；每个 `*Composition` 只装配一个功能模块的对象图。

### 3.7 assets

职责：

- 保存 Shader、纹理、模型及其他只读运行资源。
- 目录结构与课程或功能模块对应。

边界要求：

- application/domain 不依赖工程源码目录的绝对路径。
- 资源定位由 app 或 infrastructure 的资源服务完成。
- Shader 中的 uniform 和 attribute 名称属于 Shader 与调用代码之间的显式契约。

### 3.8 third_party

职责：

- 保存或导入外部依赖。
- 为外部依赖提供稳定的 CMake target。

边界要求：

- 不在第三方源码中加入项目业务逻辑。
- 不允许 application/domain 直接链接平台型第三方库。
- GLFW 当前使用 VS2022 预编译库，因此现阶段构建具有 Windows/MSVC 平台约束。

## 4. 当前 CMake 结构

项目使用 `engineeringlab_` 作为真实 target 前缀，使用 `englab::` 作为 CMake alias 前缀：

```text
englab::domain
englab::application
englab::graphics_opengl
englab::camera_galaxy
englab::concurrency
englab::ui
englab::opengl_lessons
EngineeringWorkbench
OpenGLLessons
```

当前关键依赖：

```cmake
target_link_libraries(engineeringlab_application
    PUBLIC englab::domain
    PRIVATE Threads::Threads
)

target_link_libraries(engineeringlab_graphics_opengl
    PRIVATE glad OpenGL::GL
)

target_link_libraries(engineeringlab_camera_galaxy
    PUBLIC englab::application
    PRIVATE Galaxy::SDK
)

target_link_libraries(engineeringlab_opengl_lessons
    PUBLIC
        englab::graphics_opengl
        glad
        glfw3
        OpenGL::GL
        glm::glm
        stb_image
)

target_link_libraries(EngineeringWorkbench PRIVATE
    englab::ui
    englab::application
    englab::camera_galaxy
)

target_link_libraries(OpenGLLessons PRIVATE
    englab::opengl_lessons
)
```

实际使用 `PUBLIC` 还是 `PRIVATE` 必须根据公共头文件和最终链接需求决定。当前入口拆分保持不变：`composition_root/qt_main.cpp` 生成 `EngineeringWorkbench`，`composition_root/lesson_main.cpp` 生成 `OpenGLLessons`。
## 5. CMake 边界规则

- 分层代码目录统一采用“层 / 模块 / include + src”的模块优先结构，例如 `application/camera/include`、`domain/video/include`、`infrastructure/camera/galaxy/include`。
- 每个模块优先拥有自己的 `include/` 和 `src/`；是否拆独立 `CMakeLists.txt` 和 target 根据模块规模决定。
- 每个 target 只公开模块的稳定 `include/` 根目录。
- 不把 `src/`、嵌套命名空间目录或其他模块目录加入公共 include path。
- 新模块优先显式列出源文件，避免全工程 `GLOB_RECURSE` 模糊归属。
- 外部依赖尽量使用带命名空间的 target，例如 `glm::glm`、`stb::image`。
- 只有公共头文件需要的依赖才使用 `PUBLIC`；实现细节使用 `PRIVATE`。
- 不通过全局 `include_directories()`、`link_libraries()` 或全局宏传播依赖。
- 为项目 target 建立 `englab::` 别名，减少调用方与真实 target 名称耦合。
- `ENGINEERINGLAB_ASSET_DIR` 等环境配置应由 app 或资源适配器持有，不向所有模块公开传播。

## 6. 当前阶段完成总结

本阶段完成了从“相机功能集中在 UI 层”到“相机预览最小分层切片”的迁移，并把课程运行入口整理为更适合后续扩展的导航形式。

已完成内容：

- 相机相关模型进入 `domain`：当前以 `ImageFrame` / `PixelFormat` 表达与具体相机 SDK、Qt、OpenGL 无关的稳定图像帧概念。
- 二维轨迹纯算法进入 `domain`：当前以 `ArchimedeanSpiral2DGenerator` 生成固定阿基米德螺旋上的 XOY 平面采样点，不处理文件、UI、OpenGL 或三维曲面映射。
- Qt 轨迹导出页面进入 `ui`：`TrajectoryExportView` 负责参数输入、分段编辑和后台 txt 导出；Qt Concurrent、文件写入和页面生命周期只属于外层实现，不进入 domain。
- 相机采集流程进入 `application`：`ICameraDevice` 作为外部相机能力端口，`CameraCaptureService` 使用纯 C++17 的独立控制线程、FIFO 命令队列、条件变量和 `Closed / Opened / Captured` 状态串行编排采集流程；当前 `Captured` 表示“正在采集”。每条控制命令携带一个 `std::promise<CameraResult>`，调用方通过 `std::future<CameraResult>` 获得实际执行结果；命令被拒绝、状态非法或设备操作抛出异常时也会完成对应 future。端口接口和使用它的 service 放在 application，不放 domain。
- `CameraCaptureService` 当前支持打开第一个设备、按 ID/名称打开、开始/停止采集、关闭设备，以及曝光时间、增益、帧率、自动白平衡和帧回调设置。`shutdown()` 停止接收新命令，排空已经接受的命令，然后依次注销帧回调、停止采集、关闭并销毁设备；这些流程不依赖 Qt。
- 当前学习实现保留原有的 `m_thread + queue + condition_variable + promise/future` 骨架，不额外增加关闭互斥锁或第二层命令包装。Qt 相机页面仍会在创建后自动尝试打开第一台设备并开始采集，也提供按 ID/名称打开、开始/停止/关闭和参数设置控件；UI 只调用 application 请求接口，不接触 Galaxy SDK。
- 相机 SDK 回调线程不会直接操作 QWidget/OpenGL。`CameraImageCaptureView` 使用互斥锁保护的单槽“最新帧邮箱”，只在没有待执行显示任务时向 Qt 事件队列投递一次；UI 处理不过来时新帧覆盖旧帧，避免逐帧事件无限积压。当前 UI 对控制请求返回的 future 仍立即调用 `.get()`，因此设备操作在控制线程执行，但 UI 会同步等待结果。
- 大恒相机适配进入 `infrastructure/camera/galaxy`：`GalaxyCameraController` 实现 application 端口，并用 Pimpl 隐藏大恒 SDK 头文件、句柄和回调类，避免 SDK 类型穿透公共头文件。
- Qt 组合根负责装配：`CameraComposition` 创建大恒适配器，将其作为 `ICameraDevice` 注入 `CameraCaptureService`，再把 service 注入 `CameraImageCaptureView`；`AppComposition` 负责把相机与轨迹页面注册到 `MainWindow`。
- Qt 主窗口已改为左侧导航树和右侧页面栈，后续功能页面优先独立成 QWidget 后注册到导航中。
- 相机图像显示控件支持水平翻转、垂直翻转、左右 90 度旋转、缩放、平移和重置；这些仍属于 UI 原型交互，由 `DisplayOpenGLImage` 通过 shader uniform 矩阵完成。控件显示形状可在默认矩形与圆形之间切换；圆形外观只通过 QWidget mask 裁剪并露出圆外父窗口背景。圆形模式在渲染前从原始纹理中心裁取最大正方形，并输出到居中的正方形 viewport，再对该 1:1 图像执行观察变换，避免 90 度旋转后因宽高比变化而拉伸。
- 相机控制线程、任务队列、条件变量、各把锁、原子状态、promise/future、shutdown 和最新帧机制的详细设计见[相机采集与 OpenGL 显示链路](../modules/CAMERA_ARCHITECTURE.md)。本阶段明确不继续拆分 `DisplayOpenGLImage`。
- LearnOpenGL 课程入口已形成 `LessonRegistry` 和 Qt 课程导航器；`lesson_main.cpp` 只保留命令行选择和启动导航窗口，导航窗口实现放在 `composition_root/lesson_launcher`。

本阶段仍然保留的阶段性做法：

- `DisplayOpenGLImage` 仍直接管理 Shader、VAO/VBO/EBO 和 Texture，尚未抽成 infrastructure 的 OpenGL RAII 资源。
- GLFW 教程课程仍以独立窗口运行；课程导航器通过子进程启动课程，不把 GLFW 画面直接嵌入 Qt 右侧面板。
- lessons 当前仍是单一静态库，课程源码通过递归扫描收集，尚未拆成每课程或每章节独立 target。

## 7. 已知架构债务

- `domain`、`application` 已形成相机预览所需的最小 target，但还只覆盖图像帧模型、相机端口和预览 service。
- `ui` 中的相机 SDK 依赖已拆到 `infrastructure/camera/galaxy`；OpenGL Shader、VAO/VBO/EBO、Texture 管理仍在 `DisplayOpenGLImage` 中，尚未迁移为 infrastructure RAII 资源。
- 所有课程被收集进单一 `engineeringlab_opengl_lessons` 静态库。
- 当前已形成 `lessons/catalog` 课程注册表，但还没有拆成每个课程或章节独立 target。
- lessons 的 include 目录仍由递归扫描产生，边界偏宽；infrastructure 已改为按技术能力显式列出源文件。
- 公共 include 路径已要求避免重复项目名和层名；历史或新增模块应使用 `camera/...`、`video/...`、`shader/...` 这类功能路径。
- Shader 的 OpenGL Program ID 对外公开，且尚未完整实现 RAII 和移动语义。
- GLFW 初始化、窗口创建和渲染循环在课程间重复。
- 资源路径目前通过编译期绝对路径宏传入，不利于产物搬迁。

这些问题应按真实调用点逐步处理，不为尚未出现的学习方向预建空模块或接口。
