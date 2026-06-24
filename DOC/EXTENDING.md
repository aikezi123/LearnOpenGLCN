# 扩展指南

## 1. 扩展原则

工程扩展以“保持可运行的小步迁移”为原则：一次迁移一条完整功能链路，完成后再处理下一课程。不要先创建大量抽象接口，再等待未来代码填充。

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
4. 在 `app/main.cpp` 中临时选择该课程入口。
5. 使用 Debug 配置完整构建并运行。
6. 检查窗口缩放、ESC 退出、Shader/纹理加载失败等路径。

当前 `lessons/CMakeLists.txt` 使用 `GLOB_RECURSE ... CONFIGURE_DEPENDS`，新增 `.cpp` 通常会触发重新配置。目标架构迁移后应改成显式 target 和源文件列表。

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

课程应通过注册表或命令行参数选择，而不是持续修改 `main.cpp`：

```text
LearnOpenGLCN --lesson coordinate-transform
LearnOpenGLCN --list-lessons
```

注册信息至少包含：

- 稳定课程 ID。
- 显示名称。
- 所属章节。
- 创建或运行入口。

## 4. 新增 application 用例

适用场景：新增能力描述“程序要完成什么”，并且不应绑定 OpenGL/GLFW。

步骤：

1. 在 application 定义用例的输入和输出。
2. 识别需要的外部能力，并在 application 定义最小端口接口。
3. 用 domain 类型表达核心数据，避免第三方类型穿透边界。
4. 在 infrastructure/ui 实现端口。
5. 在 app 创建具体实现并注入用例。
6. 对不需要图形上下文的应用逻辑增加单元测试。

接口应从实际用例抽取。例如，用例只需要读取时间时，定义 `IClock::elapsedSeconds()`，不要直接把整个 GLFW API 包装成一个巨型接口。

## 5. 新增 infrastructure 适配器

适用场景：接入 OpenGL、图片库、文件系统或其他外部技术。

推荐结构：

```text
infrastructure/
└── module_name/
    ├── include/learnopengl/infrastructure/module_name/
    ├── src/
    └── CMakeLists.txt
```

步骤：

1. 确认需要实现的 application 端口。
2. 将第三方依赖设置为 target 的 `PRIVATE` 依赖。
3. 避免在公共头文件暴露第三方结构体、句柄或宏。
4. 明确初始化顺序、线程要求和资源所有权。
5. 将底层错误转换为项目错误。
6. 在 app 组合根完成实例化。

对于 OpenGL RAII 对象，要特别保证对象析构发生在 GLFW Context 销毁之前。

## 6. 新增第三方依赖

新增依赖前先确认标准库或现有依赖是否已经能解决问题。

接入规则：

- 在 `third_party/<name>/CMakeLists.txt` 中建立 target，或使用 `find_package()` 导入官方 target。
- Header-only 库使用 `INTERFACE` target。
- 预编译库使用 `IMPORTED` target，并显式记录平台和工具链约束。
- 项目模块链接 target，不直接引用 `.lib`、`.dll` 或 include 绝对路径。
- 提供命名空间别名，例如 `vendor::name`。
- 更新 `DOC/ARCHITECTURE.md` 中的依赖说明。
- 检查许可证及再分发要求。

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
- 将 `#include "Shader.hpp"` 统一为稳定公开路径。
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
- 移除 `main.cpp` 对所有课程头文件的直接包含。

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

