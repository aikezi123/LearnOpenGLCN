# 代码架构与依赖关系

## 0. 长期目标与阶段策略

LearnOpenGLCN 的最终目标是系统学习并整理 LearnOpenGLCN 网站中的全部内容。课程代码首先要保持可运行、可对照教程、可逐步实验。

在学习过程中，项目会选择部分分支能力扩展为更接近工程应用的功能模块，例如：

- 工业相机图像显示：大恒、海康等相机 SDK 获取实时图像，使用 OpenGL 纹理上传与 Qt 窗口显示，目标帧率至少满足 30 FPS 以上。
- 三维轨迹与点云显示：将轨迹点、点云或空间测量数据转换为 OpenGL 可绘制数据，并通过 Qt 界面进行交互展示。
- 教程知识点复用：把 Shader、Texture、Buffer、VAO/VBO/EBO、Camera、Transform 等教程内容逐步沉淀成可复用能力。

长期方向采用整洁架构，但前期允许先在 `ui` 层完成 Qt/OpenGL 原型。原型跑通后再把稳定业务规则、用例编排和技术适配分别迁移到 domain、application、infrastructure、ui 和 app 层。文档必须区分“当前原型做法”和“目标分层做法”。

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

顶层 `CMakeLists.txt` 按以下顺序加载子目录：

1. `third_party/glad`
2. `third_party/glfw`
3. `third_party/stb`
4. `third_party/glm`
5. `third_party/Galaxy`
6. `domain`
7. `application`
8. `infrastructure`
9. `lessons`
10. `ui`
11. `composition_root`

当前目标及依赖为：

| Target | 类型 | 主要职责 | 直接依赖 |
| --- | --- | --- | --- |
| `LearnOpenGLCN_Lessons` | Executable | LearnOpenGL 课程导航器与课程代码入口 | `lessons`、Qt6 Widgets |
| `LearnOpenGLCN_Qt` | Executable | Qt/OpenGL 工程原型入口与对象装配 | `learnopengl_ui`、`infrastructure` |
| `lessons` | Static | 收集并编译所有课程示例 | `infrastructure`、`stb_image` |
| `learnopengl_ui` | Static | 当前 Qt/OpenGL 显示原型与 UI 类 | `learnopengl_application`、Qt6 Widgets/OpenGLWidgets、Qt6 OpenGL、GLAD、OpenGL、stb_image |
| `infrastructure` | Static | 提供 Shader、OpenGL 公共技术能力和大恒相机适配器 | `learnopengl_application`、GLAD、GLFW、OpenGL、GLM；实现私有依赖 Galaxy::SDK |
| `learnopengl_application` | Static | 相机预览 service 与相机端口接口 | `learnopengl_domain` |
| `learnopengl_domain` | Static | 与外部技术无关的图像帧模型和二维轨迹纯算法 | 无项目内依赖 |
| `glad` | Static | 加载 OpenGL 函数地址 | 无项目内依赖 |
| `glfw3` | Imported Static | 窗口、上下文和输入 | Windows 系统库 |
| `stb_image` | Static | 图片解码 | 无项目内依赖 |
| `glm::glm` | Interface | 数学类型和矩阵运算 | 无项目内依赖 |
| `Galaxy::SDK` | Interface | 大恒 Galaxy SDK 的 CMake 包装 target | `GxIAPI.lib`、`DxImageProc.lib`、`GxIAPICPPEx.lib` |

`third_party/Galaxy` 当前保存大恒 VC/C API、C++ SDK 头文件和 MSVC x64 import library。大恒运行时 DLL 不再由 CMake 查找或复制，而是直接放在 `out/build/<preset>/bin`，随 exe、pdb 等运行产物一起提交。

`composition_root/qt_main.cpp` 负责启动 Qt 应用并显示 UI 窗口。`composition_root/lesson_main.cpp` 负责解析命令行参数、直接运行课程或启动 LearnOpenGL 课程导航器；导航窗口实现放在 `composition_root/lesson_launcher`。课程清单集中在 `lessons/catalog` 的 `LessonRegistry` 中。所有课程源码仍被编入同一个 `lessons` 静态库，因此即使某课程没有运行，它仍必须成功编译。

当前已经拆成两个 executable：`LearnOpenGLCN_Lessons` 和 `LearnOpenGLCN_Qt`。教程入口和 Qt 工程入口不再共享同一个 `main.cpp`，避免手动改入口来切换运行内容。

`ui/` 目前包含 Qt Widgets 与 `QOpenGLWidget` 原型。`MainWindow` 已改为左侧 `QTreeWidget` 导航和右侧 `QStackedWidget` 页面容器，后续功能页面通过导航树注册。相机 SDK 控制已经从 UI 拆到 `infrastructure/camera/galaxy`，通过 `application` 中的 `ICameraDevice` 和 `CameraPreviewService` 注入到相机页面。相机页面的翻转、旋转、缩放、平移按钮属于 UI 交互，具体显示变换暂时由 `DisplayOpenGLImage` 通过 shader uniform 矩阵完成。纹理加载、Shader 编译和绘制流程仍暂时集中在 UI 类中，以便继续稳定 Qt OpenGL 显示路径；后续应再拆出 OpenGL 资源和图像加载能力，避免长期把渲染细节堆在 UI 层。

### 2.2 当前运行流程

当前 `LearnOpenGLCN_Qt` 的运行流程是 Qt 原型入口：

```text
main()
  ├── 设置 QSurfaceFormat，要求 OpenGL 3.3 Core Profile
  ├── 创建 QApplication
  ├── 创建 learnopengl::ui::MainWindow
  │   ├── 初始化左侧导航树
  │   ├── 初始化右侧页面栈
  │   └── 将 CameraImageCaptureView 注册为“相机模块 / 大恒相机预览”
  ├── 显示主窗口
  └── 进入 Qt 事件循环 app.exec()
```

当前 `LearnOpenGLCN_Lessons` 未传参数时打开 Qt 课程导航器；左侧按 LearnOpenGL 入门章节顺序列出已实现课程，右侧显示课程信息、运行按钮和子进程输出。点击运行时，导航器用子进程启动同一个 exe 并传入课程 ID；带课程 ID 参数时仍直接运行对应课程。以坐标变换课程为例，典型 GLFW 课程流程是：

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
| `LearnOpenGLCN_Lessons` | 显示课程导航器或直接运行 LearnOpenGL 教程代码 | `lessons`、Qt6 Widgets | 面向课程学习；导航器使用 Qt，课程仍允许使用 GLFW 教学式完整流程。 |
| `LearnOpenGLCN_Qt` | 运行 Qt/OpenGL 工程原型 | `learnopengl_ui` | 面向相机图像、点云、轨迹等 Qt 显示模块。 |

拆分后不再需要为了切换运行内容频繁修改同一个 `main.cpp`。两个入口都属于 `composition_root` 层，并保持很薄：

- lesson 入口只负责显示课程导航、通过课程名选择课程、调用课程函数或启动课程子进程、返回进程结果。
- Qt 入口只负责设置 Qt/OpenGL 格式、创建 `QApplication`、显示顶层窗口、进入事件循环。
- 两个入口都不能放 Shader 编译、纹理加载、相机采集、点云处理等业务或渲染细节。

当前 lesson 入口已支持 Qt 导航和命令行选择：

```powershell
LearnOpenGLCN_Lessons.exe
LearnOpenGLCN_Lessons.exe transform
LearnOpenGLCN_Lessons.exe --list
```

不带参数会打开 Qt 课程导航器；传入课程 ID 会直接运行课程；`--list` 只打印课程清单。

## 3. 目标分层职责与边界

### 3.1 domain

职责：

- 表达稳定的领域数据和规则。
- 保存与具体图形 API 无关的模型，例如颜色、变换、网格描述和场景数据。
- 执行不需要系统资源的纯计算。
- 表达“是什么”的概念，例如 `VideoFrame`、`PixelFormat`、相机设备描述、相机参数值对象等。

当前 `domain/trajectory` 包含 `ArchimedeanSpiral2DGenerator`，用于生成固定阿基米德螺旋 `r = A + Bθ` 上的二维采样点。该算法只使用标准库，只控制 XOY 平面点距；线间距全局固定并用于计算 `B`，不同半径段只改变目标点间距。

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

- 编排用例和程序流程；本项目应用层流程对象统一采用 `Service` 命名，例如 `CameraPreviewService`。
- 定义外部能力端口，例如 `ICameraDevice`、`IRenderer`、`IImageLoader`、`IClock`。
- 使用 `domain` 模型表达输入和输出。
- 定义与具体 OpenGL 实现无关的课程运行协议。

端口接口应和使用它的 application service 放在同一层。application 通过端口描述“为了完成这个流程，需要外部世界提供什么能力”；具体实现由 infrastructure 提供。比如 `CameraPreviewService` 使用 `ICameraDevice`，而大恒的 `GalaxyCameraController` 在 infrastructure 中实现 `ICameraDevice`。

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

当前入口目录名为 `composition_root/`，承担 app/Composition Root 角色。不要在 `main.cpp` 中堆积业务逻辑、Shader 编译、纹理加载或相机采集流程。

术语约定：

- `application` 定义端口，例如 `ICameraDevice`。
- `infrastructure` 继承并重写端口，例如 `GalaxyCameraController : ICameraDevice`，这叫“实现端口”。
- `composition_root` 创建具体实现，并把它按端口类型传给需要该依赖的 service，例如把 `GalaxyCameraController` 作为 `ICameraDevice` 传给 `CameraPreviewService`，这个“从外部传入依赖”的动作叫“依赖注入”。

依赖注入不是消除依赖，而是让 service 依赖抽象端口，把“使用哪个具体实现”的决定权移动到组合根。

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

## 4. 目标 CMake 结构

每一层应该由独立 target 表达，而不是只依靠目录名：

```text
learnopengl::domain
learnopengl::application
learnopengl::opengl
learnopengl::image
learnopengl::ui_glfw
learnopengl::ui
learnopengl::lessons
LearnOpenGLCN_Lessons
LearnOpenGLCN_Qt
```

建议依赖关系：

```cmake
target_link_libraries(learnopengl_application
    PUBLIC learnopengl::domain
)

target_link_libraries(learnopengl_opengl
    PUBLIC learnopengl::application
    PRIVATE glad OpenGL::GL glm::glm
)

target_link_libraries(learnopengl_image
    PUBLIC learnopengl::application
    PRIVATE stb::image
)

target_link_libraries(learnopengl_ui_glfw
    PUBLIC learnopengl::application
    PRIVATE glfw3
)

target_link_libraries(LearnOpenGLCN_Lessons PRIVATE
    learnopengl::lessons
    learnopengl::opengl
    learnopengl::image
    learnopengl::ui_glfw
)

target_link_libraries(LearnOpenGLCN_Qt PRIVATE
    learnopengl::ui
)
```

实际使用 `PUBLIC` 还是 `PRIVATE` 必须根据公共头文件是否暴露依赖决定，不能机械照抄示例。

当前工程已经完成入口拆分。`composition_root/qt_main.cpp` 生成 `LearnOpenGLCN_Qt`，`composition_root/lesson_main.cpp` 生成 `LearnOpenGLCN_Lessons`，两个 executable 都输出到同一个 `bin` 目录。

## 5. CMake 边界规则

- 分层代码目录统一采用“层 / 模块 / include + src”的模块优先结构，例如 `application/camera/include`、`domain/video/include`、`infrastructure/camera/galaxy/include`。
- 每个模块优先拥有自己的 `include/` 和 `src/`；是否拆独立 `CMakeLists.txt` 和 target 根据模块规模决定。
- 每个 target 只公开模块的稳定 `include/` 根目录。
- 不把 `src/`、嵌套命名空间目录或其他模块目录加入公共 include path。
- 新模块优先显式列出源文件，避免全工程 `GLOB_RECURSE` 模糊归属。
- 外部依赖尽量使用带命名空间的 target，例如 `glm::glm`、`stb::image`。
- 只有公共头文件需要的依赖才使用 `PUBLIC`；实现细节使用 `PRIVATE`。
- 不通过全局 `include_directories()`、`link_libraries()` 或全局宏传播依赖。
- 为项目 target 建立 `learnopengl::` 别名，减少调用方与真实 target 名称耦合。
- `LEARNOPENGL_ASSET_DIR` 等环境配置应由 app 或资源适配器持有，不向所有模块公开传播。

## 6. 当前阶段完成总结

本阶段完成了从“相机功能集中在 UI 层”到“相机预览最小分层切片”的迁移，并把课程运行入口整理为更适合后续扩展的导航形式。

已完成内容：

- 相机相关模型进入 `domain`：当前以 `VideoFrame` / `PixelFormat` 表达与具体相机 SDK、Qt、OpenGL 无关的稳定图像帧概念。
- 二维轨迹纯算法进入 `domain`：当前以 `ArchimedeanSpiral2DGenerator` 生成固定阿基米德螺旋上的 XOY 平面采样点，不处理文件、UI、OpenGL 或三维曲面映射。
- 相机预览流程进入 `application`：`ICameraDevice` 作为外部相机能力端口，`CameraPreviewService` 负责编排预览流程。端口接口和使用它的 service 放在 application，不放 domain。
- 大恒相机适配进入 `infrastructure/camera/galaxy`：`GalaxyCameraController` 实现 application 端口，并用 Pimpl 隐藏大恒 SDK 头文件、句柄和回调类，避免 SDK 类型穿透公共头文件。
- Qt 组合根负责装配：`composition_root/qt_main.cpp` 创建大恒适配器，将其作为 `ICameraDevice` 注入 `CameraPreviewService`，再把 service 注入 `MainWindow` / `CameraImageCaptureView`。
- Qt 主窗口已改为左侧导航树和右侧页面栈，后续功能页面优先独立成 QWidget 后注册到导航中。
- 相机图像显示控件支持水平翻转、垂直翻转、左右 90 度旋转、缩放、平移和重置；这些仍属于 UI 原型交互，由 `DisplayOpenGLImage` 通过 shader uniform 矩阵完成。
- LearnOpenGL 课程入口已形成 `LessonRegistry` 和 Qt 课程导航器；`lesson_main.cpp` 只保留命令行选择和启动导航窗口，导航窗口实现放在 `composition_root/lesson_launcher`。

本阶段仍然保留的阶段性做法：

- `DisplayOpenGLImage` 仍直接管理 Shader、VAO/VBO/EBO 和 Texture，尚未抽成 infrastructure 的 OpenGL RAII 资源。
- GLFW 教程课程仍以独立窗口运行；课程导航器通过子进程启动课程，不把 GLFW 画面直接嵌入 Qt 右侧面板。
- lessons 当前仍是单一静态库，课程源码通过递归扫描收集，尚未拆成每课程或每章节独立 target。

## 7. 已知架构债务

- `domain`、`application` 已形成相机预览所需的最小 target，但还只覆盖图像帧模型、相机端口和预览 service。
- `ui` 中的相机 SDK 依赖已拆到 `infrastructure/camera/galaxy`；OpenGL Shader、VAO/VBO/EBO、Texture 管理仍在 `DisplayOpenGLImage` 中，尚未迁移为 infrastructure RAII 资源。
- 所有课程被收集进单一 `lessons` 静态库。
- 当前已形成 `lessons/catalog` 课程注册表，但还没有拆成每个课程或章节独立 target。
- lessons 和 infrastructure 的 include 目录由递归扫描产生，边界过宽。
- 公共 include 路径已要求避免重复项目名和层名；历史或新增模块应使用 `camera/...`、`video/...`、`shader/...` 这类功能路径。
- Shader 的 OpenGL Program ID 对外公开，且尚未完整实现 RAII 和移动语义。
- GLFW 初始化、窗口创建和渲染循环在课程间重复。
- 资源路径目前通过编译期绝对路径宏传入，不利于产物搬迁。

这些问题应采用纵向切片逐步迁移，不建议一次性重写全部课程。
