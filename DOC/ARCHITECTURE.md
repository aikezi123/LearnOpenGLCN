# 代码架构与依赖关系

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
       │ GLFW/Input │   │ OpenGL/stb/FS  │   │ 功能模块   │
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

`third_party/` 和操作系统 OpenGL 库只允许被外层适配器使用，不能向 `application` 或 `domain` 泄漏。

## 2. 当前实现

### 2.1 构建结构

顶层 `CMakeLists.txt` 按以下顺序加载子目录：

1. `third_party/glad`
2. `third_party/glfw`
3. `third_party/stb`
4. `third_party/glm`
5. `infrastructure`
6. `lessons`
7. `app`

当前目标及依赖为：

| Target | 类型 | 主要职责 | 直接依赖 |
| --- | --- | --- | --- |
| `LearnOpenGLCN` | Executable | 程序入口，选择要运行的课程 | `lessons` |
| `lessons` | Static | 收集并编译所有课程示例 | `infrastructure`、`stb_image` |
| `infrastructure` | Static | 提供 Shader 等公共技术能力 | GLAD、GLFW、OpenGL、GLM |
| `glad` | Static | 加载 OpenGL 函数地址 | 无项目内依赖 |
| `glfw3` | Imported Static | 窗口、上下文和输入 | Windows 系统库 |
| `stb_image` | Static | 图片解码 | 无项目内依赖 |
| `glm::glm` | Interface | 数学类型和矩阵运算 | 无项目内依赖 |

`app/main.cpp` 当前通过直接调用某个课程函数选择运行内容。所有课程源码被编入同一个 `lessons` 静态库，因此即使某课程没有运行，它仍必须成功编译。

### 2.2 当前运行流程

以坐标变换课程为例，运行过程为：

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

## 3. 目标分层职责与边界

### 3.1 domain

职责：

- 表达稳定的领域数据和规则。
- 保存与具体图形 API 无关的模型，例如颜色、变换、网格描述和场景数据。
- 执行不需要系统资源的纯计算。

允许依赖：

- C++ 标准库。
- `domain` 内部其他模块。

禁止依赖：

- OpenGL、GLAD、GLFW、stb_image。
- 文件路径、窗口句柄和 GPU 对象 ID。
- `application`、`infrastructure`、`ui`、`lessons`、`app`。

领域模型不足时允许该层保持精简，不为满足目录形式而制造无意义抽象。

### 3.2 application

职责：

- 编排用例和程序流程。
- 定义外部能力端口，例如 `IRenderer`、`IWindow`、`IImageLoader`、`IClock`。
- 使用 `domain` 模型表达输入和输出。
- 定义与具体 OpenGL 实现无关的课程运行协议。

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

边界要求：

- OpenGL 对象必须有清晰的所有权，优先使用 RAII。
- 具体实现可以依赖 application/domain，反向依赖不允许出现。
- 第三方类型尽量停留在 `.cpp` 或 infrastructure 的私有头文件中。
- 不在该层决定具体运行哪一课程。

### 3.4 ui

职责：

- 创建和管理窗口。
- 读取键盘、鼠标及窗口事件。
- 将平台事件转换为 application 能理解的输入。
- 后续可承载课程选择界面或调试 UI。

边界要求：

- GLFW 类型不得进入 domain。
- UI 不直接实现领域规则。
- UI 触发 application 用例，而不是反向控制 application 内部状态。

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
learnopengl::lessons
LearnOpenGLCN
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

target_link_libraries(LearnOpenGLCN PRIVATE
    learnopengl::lessons
    learnopengl::opengl
    learnopengl::image
    learnopengl::ui_glfw
)
```

实际使用 `PUBLIC` 还是 `PRIVATE` 必须根据公共头文件是否暴露依赖决定，不能机械照抄示例。

## 5. CMake 边界规则

- 每个模块拥有自己的 `CMakeLists.txt` 和 target。
- 每个 target 只公开一个稳定的 `include/` 根目录。
- 不把 `src/`、嵌套命名空间目录或其他模块目录加入公共 include path。
- 新模块优先显式列出源文件，避免全工程 `GLOB_RECURSE` 模糊归属。
- 外部依赖尽量使用带命名空间的 target，例如 `glm::glm`、`stb::image`。
- 只有公共头文件需要的依赖才使用 `PUBLIC`；实现细节使用 `PRIVATE`。
- 不通过全局 `include_directories()`、`link_libraries()` 或全局宏传播依赖。
- 为项目 target 建立 `learnopengl::` 别名，减少调用方与真实 target 名称耦合。
- `LEARNOPENGL_ASSET_DIR` 等环境配置应由 app 或资源适配器持有，不向所有模块公开传播。

## 6. 已知架构债务

- `domain`、`application`、`ui` 仍为空，尚未形成真实依赖层。
- 所有课程被收集进单一 `lessons` 静态库。
- 切换课程需要手动修改 `main.cpp`。
- lessons 和 infrastructure 的 include 目录由递归扫描产生，边界过宽。
- 部分课程使用 `"Shader.hpp"`，依赖泄漏的 include 搜索路径。
- Shader 的 OpenGL Program ID 对外公开，且尚未完整实现 RAII 和移动语义。
- GLFW 初始化、窗口创建和渲染循环在课程间重复。
- 资源路径目前通过编译期绝对路径宏传入，不利于产物搬迁。

这些问题应采用纵向切片逐步迁移，不建议一次性重写全部课程。

