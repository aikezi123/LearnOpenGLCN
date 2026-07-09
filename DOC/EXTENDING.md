# 扩展指南

## 1. 扩展原则

工程扩展以“保持可运行的小步迁移”为原则：一次迁移一条完整功能链路，完成后再处理下一课程。不要先创建大量抽象接口，再等待未来代码填充。

扩展时优先实现当前功能真正需要的代码。不要添加未被当前业务路径使用的预留宏、feature flag、CMake option、接口或包装层；只有当前收益明确、后续使用位置清楚时，才提前加入这类扩展点。

本项目有两类扩展：

1. LearnOpenGLCN 教程学习扩展：按章节继续实现网站中的课程与练习。
2. 工程功能扩展：从某个 OpenGL 知识点出发，扩展为实际功能模块，例如相机实时图像显示、三维轨迹或点云绘制。

前期允许在 `ui` 层先做完整 Qt/OpenGL 原型，尤其是需要理解 Qt 事件循环、`QOpenGLWidget` 生命周期和渲染流程时。原型跑通后再把稳定逻辑拆分到 application/infrastructure/domain。不要把“先跑通”的临时代码误写成最终分层方案。

每次扩展应回答三个问题：

1. 新代码属于稳定业务规则，还是外部技术细节？
2. 它需要依赖哪些模块，依赖方向是否指向内层？
3. 它拥有哪种资源，资源在什么时间、由谁释放？

## 2. 当前阶段新增课程

在整洁架构迁移完成前，可以沿用现有 lessons 组织方式。

建议目录：

```text
lessons/
└── getting_started/
    ├── include/
    │   └── lesson_name.h
    └── src/
        └── lesson_name.cpp
```

资源目录：

```text
assets/
├── shaders/getting_started/lesson_name/
└── textures/getting_started/lesson_name/
```

步骤：

1. 创建课程公开入口头文件。
2. 在 `.cpp` 中实现窗口初始化、资源创建、渲染循环和清理。
3. 资源路径以 `assets` 为根组织，不依赖当前工作目录。
4. 在 `lessons/catalog/src/LessonRegistry.cpp` 的课程注册表中加入该课程入口，保持顺序与 LearnOpenGL 目录一致。
5. 使用 Debug 配置完整构建并运行。
6. 检查窗口缩放、ESC 退出、Shader/纹理加载失败等路径。

当前 `lessons/CMakeLists.txt` 使用 `GLOB_RECURSE ... CONFIGURE_DEPENDS`，新增 `.cpp` 通常会触发重新配置。目标架构迁移后应改成显式 target 和源文件列表。

当前 `LearnOpenGLCN_Lessons.exe` 不带参数会打开 Qt 课程导航器；选择课程后，导航器通过子进程传入课程 ID 来运行对应 GLFW 课程窗口。导航窗口代码位于 `composition_root/lesson_launcher`，`lesson_main.cpp` 只保留参数解析、直接运行课程和启动导航窗口。传入课程 ID 时仍可直接运行课程，例如 `LearnOpenGLCN_Lessons.exe transform`；`--list` 可查看已注册课程。

## 3. 目标阶段新增课程

建议把每个课程或章节组建成独立 target：

```cmake
add_library(lesson_coordinate_transform STATIC)

target_sources(lesson_coordinate_transform PRIVATE
    src/CoordinateTransformationLesson.cpp
)

target_include_directories(lesson_coordinate_transform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lesson_coordinate_transform
    PUBLIC learnopengl::application
    PRIVATE learnopengl::opengl
)

add_library(learnopengl::lesson_coordinate_transform
    ALIAS lesson_coordinate_transform
)
```

如果课程直接教授 OpenGL API，允许它私有依赖 OpenGL 适配器；如果课程只表达用例，则只依赖 application。

课程应通过课程入口列表或命令行参数选择，而不是持续修改 Qt 入口：

```text
LearnOpenGLCN_Lessons.exe transform
LearnOpenGLCN_Lessons.exe --list
```

注册信息至少包含：

- 稳定课程 ID。
- 显示名称。
- 所属章节。
- 创建或运行入口。

## 4. 新增 application service

适用场景：新增能力描述“程序要完成什么”，并且不应绑定 OpenGL/GLFW。

推荐结构：

```text
application/
└── module_name/
    ├── include/module_name/
    └── src/
```

步骤：

1. 在 application 定义 service 的输入和输出，命名采用 `XxxService`。
2. 识别需要的外部能力，并在 application 定义最小端口接口。
3. 用 domain 类型表达核心数据，避免第三方类型穿透边界。
4. 在 infrastructure/ui 实现端口。
5. 在 app 创建具体实现并注入 service。
6. 对不需要图形上下文的应用逻辑增加单元测试。

接口应从实际 service 抽取。例如，service 只需要读取时间时，定义 `IClock::elapsedSeconds()`，不要直接把整个 GLFW API 包装成一个巨型接口。

端口接口放在 application，而不是 domain。判断方式：如果类型表达“是什么”，例如图像帧、像素格式、相机参数值对象，放 domain；如果接口表达“为了完成某个流程，需要外部系统做什么”，例如 `ICameraDevice`、`IImageLoader`、`IRenderer`，放 application；如果代码表达“用某个 SDK/API 具体怎么做”，放 infrastructure。

术语区分：

- 外层类继承并重写 application 端口，叫实现端口。例如 `GalaxyCameraController : ICameraDevice`。
- 组合根创建具体实现，并按端口类型传给 service，叫依赖注入。例如把 `GalaxyCameraController` 作为 `ICameraDevice` 注入 `CameraPreviewService`。
- 依赖注入不是让类之间没有依赖，而是让 service 依赖抽象端口，不直接依赖具体实现。

## 4.1 从 UI 原型迁移到分层实现

当一个功能先在 `ui` 层跑通后，按以下顺序拆分：

1. 保留 `ui` 对窗口、控件、事件和 `QOpenGLWidget` 生命周期的管理。
2. 把稳定的流程编排提取为 application service，例如“接收一帧图像并请求显示”或“提交一批轨迹点并请求绘制”。
3. 把外部技术实现放入 infrastructure，例如相机 SDK 适配器、stb_image 加载器、OpenGL Texture/Buffer 封装。
4. 把与 UI/API 无关的数据模型放入 domain，例如帧尺寸、像素格式、点云点、轨迹段、相机内参等。
5. `composition_root` 负责把 Qt UI、OpenGL 适配器、相机适配器和 application service 装配起来。

当前 Qt 主窗口使用左侧 `QTreeWidget` 导航和右侧 `QStackedWidget` 页面容器。新增 UI 页面时，优先把页面实现为独立 QWidget，再在 `MainWindow::initPages()` 中通过导航节点注册，不要继续把多个功能直接堆到同一个窗口控件里。

拆分过程中保持每一步可构建、可运行，不一次性重写全部原型。

## 5. 新增 infrastructure 适配器

适用场景：接入 OpenGL、图片库、文件系统或其他外部技术。

推荐结构：

```text
infrastructure/
└── module_name/
    ├── include/module_name/
    ├── src/
    └── CMakeLists.txt
```

分层代码统一采用“层 / 模块 / include + src”的模块优先结构。比如 `application/camera/include/camera/`、`domain/video/include/video/`、`infrastructure/camera/galaxy/include/camera/galaxy/`。公共 include 目录下不要重复项目名或当前层名，不要创建 `learnopengl/application/camera/`、`learnopengl/domain/video/` 这类目录。

步骤：

1. 确认需要实现的 application 端口。
2. 将第三方依赖设置为 target 的 `PRIVATE` 依赖。
3. 避免在公共头文件暴露第三方结构体、句柄或宏；相机 SDK、平台 API 等适配器优先用 Pimpl 把 SDK 成员和回调类藏到 `.cpp` 中。
4. 明确初始化顺序、线程要求和资源所有权。
5. 将底层错误转换为项目错误。
6. 在 app 组合根完成实例化。

对于 OpenGL RAII 对象，要特别保证对象析构发生在 GLFW Context 销毁之前。

对于 Qt 场景，OpenGL 资源的创建和销毁必须发生在 `QOpenGLWidget` 的有效 context 中。可以先让 UI 持有资源；稳定后再把资源类型提取到 infrastructure，但 context 所有权和线程要求必须在接口或调用约定中写清楚。

## 5.1 相机图像显示模块

相机显示的推荐演进路线：

1. 原型阶段：在 `QOpenGLWidget` 中使用一张本地图片验证纹理显示。
2. 单帧阶段：使用 stb_image 或模拟相机数据上传到 OpenGL Texture。
3. 实时阶段：接入大恒/海康 SDK，基础设施层将相机回调或拉流数据转换为统一帧数据。
4. 渲染阶段：首次创建纹理时用 `glTexImage2D` 分配存储，后续每帧用 `glTexSubImage2D` 更新。
5. 分层阶段：application 定义帧流端口，infrastructure 实现具体相机，ui 只显示结果和处理交互。

注意：

- domain/application 不依赖大恒、海康、Qt、OpenGL 或 stb_image。
- 相机 SDK 类型不得穿透到 application/domain 公共接口。
- 帧格式、宽高、stride、通道顺序、线程和缓冲区所有权必须明确。
- 目标帧率至少满足 30 FPS，避免每帧重复创建纹理或 Program。

当前大恒实现为 `learnopengl::infrastructure::camera::galaxy::GalaxyCameraController`，实现 `application::ICameraDevice`。它负责大恒 SDK 初始化、开关相机和输出 `domain::VideoFrame`，SDK 头文件只出现在 infrastructure 的 `.cpp` 中。`CameraImageCaptureView` 通过 `CameraPreviewService` 接收帧，并用 Qt queued invoke 投递到 UI 线程；OpenGL 上传仍放在 `QOpenGLWidget` 的有效 context 中完成，不在相机 SDK 回调线程中直接调用 `glXXX`。当前图像翻转、旋转、缩放和平移属于 UI 原型交互，由 `CameraImageCaptureView` 控件发起，由 `DisplayOpenGLImage` 在绘制前上传 shader uniform 矩阵完成。

## 5.2 三维轨迹与点云显示模块

轨迹和点云显示的推荐演进路线：

1. 原型阶段：在 `QOpenGLWidget` 中直接绘制点、线或简单 VAO/VBO。
2. 数据模型阶段：将点、颜色、时间戳、轨迹段等与 OpenGL 无关的数据放入 domain。
3. 用例阶段：application 负责接收数据、选择显示策略、控制更新节奏。
4. 渲染适配阶段：infrastructure 将点云或轨迹数据上传到 OpenGL Buffer。
5. UI 阶段：Qt 负责视图控制、交互、缩放、旋转、选择和调试面板。

不要让 domain 保存 OpenGL Buffer ID，也不要让 application 创建 QWidget 或调用 `glXXX`。

## 6. 新增第三方依赖

新增依赖前先确认标准库或现有依赖是否已经能解决问题。

接入规则：

- 在 `third_party/<name>/CMakeLists.txt` 中建立 target，或使用 `find_package()` 导入官方 target。
- Header-only 库使用 `INTERFACE` target。
- 预编译库使用 `IMPORTED` target，并显式记录平台和工具链约束。
- 项目模块链接 target，不直接引用 `.lib`、`.dll` 或 include 绝对路径。
- 提供命名空间别名，例如 `vendor::name`。
- 不为第三方依赖添加当前没有源码消费点的编译宏或配置开关。
- 更新 `DOC/ARCHITECTURE.md` 中的依赖说明。
- 检查许可证及再分发要求。

当前大恒 Galaxy SDK 采用 `third_party/Galaxy/CMakeLists.txt` 包装为 `Galaxy::SDK`。推荐目录为：

```text
third_party/Galaxy/
├── CMakeLists.txt
├── include/
│   ├── VC_SDK/
│   │   ├── GxIAPI.h
│   │   └── DxImageProc.h
│   └── C++_SDK/
│       └── GalaxyIncludes.h
├── libs/
│   ├── VC_Lib/
│   │   ├── GxIAPI.lib
│   │   └── DxImageProc.lib
│   └── VC++_Lib/
│       └── GxIAPICPPEx.lib
```

现阶段 `Galaxy::SDK` 同时暴露大恒 VC/C API 和 C++ SDK 头文件，并链接 `GxIAPI.lib`、`DxImageProc.lib`、`GxIAPICPPEx.lib`。`.lib` 只解决链接；运行时 DLL 直接放在 `out/build/<preset>/bin`，不再通过环境变量或 CMake 复制逻辑定位。

## 7. 新增资源

资源应按功能而不是文件格式随意堆放：

```text
assets/
├── shaders/<chapter>/<lesson>/
├── textures/<chapter>/<lesson>/
└── models/<chapter>/<lesson>/
```

新增资源时：

- 使用小写、清晰且稳定的目录名。
- 避免同一资源产生多个含义不明的副本。
- 在代码中通过资源定位服务获取路径。
- 对缺失、损坏和不支持格式提供明确错误。
- 如果未来复制资源到构建目录，由 CMake 集中完成，不在各课程中执行复制命令。

## 8. 推荐迁移路线

### 阶段一：收紧现有边界

- 修正 include 目录，不再公开 `src` 和嵌套目录。
- 将 `#include "Shader.hpp"` 统一为稳定公开路径，例如 `#include <shader/Shader.hpp>`。
- 为现有 target 增加 `learnopengl::` 别名。
- 将不必要的 `PUBLIC` 依赖改为 `PRIVATE`。

验收结果：现有课程行为不变，但错误的跨模块 include 会在编译期暴露。

### 阶段二：建立资源所有权

- 将 Shader Program 改成不可复制、可移动的 RAII 类型。
- 增加 Buffer、VertexArray、Texture 封装。
- 统一 Shader uniform 设置接口，包括矩阵类型。
- 确保 Context 最后销毁。

验收结果：课程正常退出时不存在由项目代码遗漏的 GPU 资源。

### 阶段三：建立 application 与端口

- 从一个课程中提取最小的 `IWindow`、`IClock` 或渲染端口。
- 在 ui/infrastructure 中提供 GLFW/OpenGL 实现。
- 保持其他课程继续使用旧结构。

验收结果：选定课程形成一条 `app → outer adapters → application → domain` 的完整纵向切片。

### 阶段四：课程模块化

- 每个课程或章节建立独立 target。
- 增加课程注册表和命令行选择。
- 移除 `lesson_main.cpp` 对所有课程头文件的直接包含，改为更明确的课程注册机制。

验收结果：一个课程损坏不会阻止无关课程被单独构建和测试。

### 阶段五：可移植资源与平台

- 构建时将 assets 部署到运行目录，或配置统一资源根目录。
- 将预编译 GLFW 库替换为可配置的包或源码构建方案。
- 增加其他编译器和平台的 CMake preset。

验收结果：构建产物不依赖开发机器上的源码绝对路径。

## 9. 扩展完成标准

一项新增功能完成时至少满足：

- CMake target 依赖方向符合架构规则。
- Debug 构建通过，并完成最小运行验证。
- 公共头文件不泄漏不必要的第三方类型。
- 所有资源具有明确所有者和释放时机。
- 失败路径提供可定位的错误信息。
- 新增目录、模块或架构决策已同步到 `DOC/`。
- 没有为了当前功能顺便重写无关课程。
