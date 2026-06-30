# LearnOpenGLCN 项目协作规则

本文件适用于仓库根目录及所有子目录。Codex 在本项目中分析、修改、构建或验证代码时必须遵守这些规则。若用户当前请求与本文件冲突，先指出冲突并请求确认；不要静默突破架构边界或扩大修改范围。

## 1. 项目背景

LearnOpenGLCN 是一个使用 C++17、CMake 3.21+、Ninja、MSVC、OpenGL 和 Qt 学习并整理 LearnOpenGLCN 网站内容的工程。

长期目标：

- 学习完 LearnOpenGLCN 网站中的全部章节、示例和练习。
- 保持教程代码可运行、可对照、可实验。
- 在部分学习分支上扩展工程功能模块，例如：
  - 大恒/海康工业相机实时图像采集，并使用 Qt + OpenGL 显示。
  - 三维轨迹数据、点云数据的 OpenGL 绘制与 Qt 交互显示。
  - Shader、Texture、Buffer、VAO/VBO/EBO、Camera、Transform 等能力的复用封装。

当前工程已经接入 Qt UI 层。`ui/` 形成 `learnopengl_ui` target，使用 Qt Widgets、Qt OpenGLWidgets、GLAD、OpenGL 和 stb_image。当前仍处于学习和原型阶段，允许先在 UI 层跑通 Qt/OpenGL 功能闭环，再逐步拆分到整洁架构各层。

当前主要 target 关系：

```text
LearnOpenGLCN_Lessons (executable)
    └── lessons (STATIC)
        ├── infrastructure (STATIC)
        │   ├── glad
        │   ├── glfw3
        │   ├── OpenGL::GL
        │   └── glm::glm
        └── stb_image

LearnOpenGLCN_Qt (executable)
    └── learnopengl_ui (STATIC)
        ├── Qt6::Widgets
        ├── Qt6::OpenGLWidgets
        ├── Qt6::OpenGL
        ├── glad
        ├── OpenGL::GL
        └── stb_image
```

`domain/` 和 `application/` 已有目录，但当前尚未形成稳定 target。不要把目标分层描述成已经全部完成。

## 2. 修改前优先阅读

较大修改前按顺序阅读：

1. 根目录 `AGENTS.md`
2. `.agents/AGENTS.md`
3. `.agents/CODEX_CONTEXT.md`
4. `DOC/README.md`
5. `DOC/ARCHITECTURE.md`
6. `DOC/CODING_STYLE.md`
7. `DOC/EXTENDING.md`
8. 顶层 `CMakeLists.txt` 和 `CMakePresets.json`
9. 被修改模块及其直接依赖模块的 `CMakeLists.txt`
10. 被修改模块的公开头文件、实现文件、Shader、纹理和资源文件

修改前还必须：

- 运行 `git status --short`，识别用户已有改动。
- 不覆盖、不删除、不还原用户已有修改。
- 搜索现有同类实现和命名，避免重复抽象。
- 明确本次修改属于教程学习、UI 原型、基础设施适配还是架构迁移。
- 采用最小必要实现：不添加与当前任务、当前业务路径或当前调用点无关的预留代码。
- 只有在收益明确且后续使用概率很高时，才添加提前设计的扩展点、编译宏、CMake option、接口或抽象；否则等真实需求出现后再补。
- 多文件、跨层、CMake 或公共接口变更前先给出简短计划。

## 3. 整洁架构分层规则

长期依赖方向必须指向内层：

```text
composition_root/app
    -> ui
    -> infrastructure
    -> application
    -> domain
```

基本规则：

- `domain` 不依赖任何外层模块或外部技术。
- `application` 定义用例和端口，不依赖 Qt、OpenGL、GLFW、stb_image、相机 SDK。
- `infrastructure` 实现外部技术细节，例如 OpenGL、图片加载、文件系统、相机 SDK。
- `ui` 管理窗口、控件、事件和可视化交互。
- `composition_root` 负责创建对象、注入依赖、启动程序。
- 第三方类型、Qt 类型、OpenGL ID、相机 SDK 类型不得泄漏到 `domain` 或 `application`。

阶段策略：

- 当前前期可以在 `ui` 层先完整实现 Qt/OpenGL 原型。
- UI 原型跑通后，稳定流程再迁移到 application/infrastructure/domain。
- 不要把“临时 UI 大杂烩”写成长期架构建议。

## 4. 各层职责与禁止事项

### 4.1 domain

允许：

- 表达与图形 API 无关的数据模型和规则。
- 表达颜色、变换、网格描述、点云点、轨迹段、图像帧元数据等稳定概念。
- 使用 C++ 标准库和 domain 内部代码。

禁止：

- 依赖 Qt、OpenGL、GLAD、GLFW、GLM、stb_image、大恒/海康 SDK。
- 保存 QWidget、QOpenGLWidget、OpenGL ID、相机 SDK 句柄、文件系统适配器。
- 调用 `glXXX` 或任何 UI/API 相关函数。

### 4.2 application

允许：

- 编排用例和流程。
- 定义端口，例如图像帧源、渲染提交、时钟、资源定位、点云数据输入。
- 使用 domain 类型描述输入、输出和状态。

禁止：

- 创建 QWidget、QOpenGLWidget、QApplication。
- 调用 `glXXX`、GLFW、Qt API、stb_image 或相机 SDK。
- include Qt、GLAD、GLFW、stb_image、相机 SDK 头文件。
- 知道 infrastructure/ui 的具体类。

### 4.3 infrastructure

允许：

- 实现 application 定义的技术端口。
- 封装 OpenGL Shader、Texture、Buffer、VertexArray、Program。
- 使用 stb_image 加载图片。
- 接入大恒、海康等相机 SDK。
- 处理文件系统、资源路径和底层错误转换。

禁止：

- 把第三方类型、OpenGL ID、SDK 句柄泄漏到 domain/application 公共接口。
- 在没有有效 OpenGL Context 时创建、使用或销毁 GPU 对象。
- 决定 UI 布局、课程选择或业务用例流程。

### 4.4 ui

允许：

- 使用 Qt Widgets、QOpenGLWidget、Qt Designer `.ui`、信号槽和 Qt 事件循环。
- 管理窗口、控件、输入事件、交互和可视化展示。
- 当前原型阶段可直接创建 Shader、VAO/VBO/EBO、Texture，跑通 OpenGL 显示流程。
- 将 UI 事件转换为 application 能理解的输入。

禁止：

- 让 Qt 类型进入 domain/application。
- 让 application 直接依赖 QWidget、QOpenGLWidget 或具体窗口类。
- 长期在 UI 层堆积相机采集、点云处理、业务规则和资源管理。
- 同时维护互相竞争的 Qt 和 GLFW 主事件循环。

### 4.5 composition_root / app

允许：

- 创建 QApplication。
- 设置必要的 OpenGL 默认格式。
- 创建顶层窗口。
- 装配 ui、infrastructure、application 对象。
- 进入事件循环。

禁止：

- 在 `main.cpp` 中塞业务逻辑、Shader 编译、纹理加载、相机采集或大段渲染代码。
- 让其他层依赖 composition_root。

### 4.6 lessons

允许：

- 按 LearnOpenGLCN 章节组织可运行课程示例。
- 教学型课程直接展示 OpenGL 调用。
- 保留适合学习的完整初始化、渲染和清理流程。

禁止：

- 为修改一个课程顺手重写全部课程。
- 长期复制稳定的资源所有权逻辑；出现重复后应提出小步抽取方案。
- 新增含义过宽的全局入口名。

## 5. CMake 修改规则

- 保持 CMake 3.21+ 和 C++17，除非用户明确要求升级。
- 使用 target-based CMake。
- 不新增全局 `include_directories()`、`link_libraries()` 或无关全局宏。
- 不新增未被当前源码消费的编译宏、CMake option、feature flag 或预留开关；确有必要时必须说明当前收益和预计使用位置。
- 只有公共头文件需要的依赖才能设为 `PUBLIC`；实现细节使用 `PRIVATE`。
- domain/application target 不得链接 Qt、GLAD、GLFW、OpenGL、stb_image 或相机 SDK。
- Qt 依赖只允许放在 ui 或明确的外层适配器 target。
- OpenGL/stb_image/相机 SDK 依赖只允许放在 infrastructure/ui 等外层实现。
- 不硬编码本机路径、Visual Studio 安装路径或 Qt 安装路径。
- 不新增与任务无关的工具链、包管理器、preset 或外部依赖。
- 修改 target 关系后同步检查顶层 `add_subdirectory()` 顺序。

当前构建入口以 `CMakePresets.json` 和 `msvc-cmake.ps1` 为准。

## 6. C++ 编码规则

- 使用 C++17 和 UTF-8。
- 项目代码优先放入 `learnopengl` 根命名空间；当前原型遗留代码可逐步迁移。
- 类型和枚举使用 PascalCase。
- 函数和局部变量使用 camelCase。
- 成员变量使用 `m_` + camelCase。
- 头文件使用 `#pragma once` 或唯一 include guard。
- 头文件禁止 `using namespace`。
- include 顺序：对应头文件、项目头文件、第三方头文件、标准库头文件。
- 函数保持单一职责。
- 错误信息应包含操作、资源路径或对象类型。
- 不对无关文件做批量格式化、重命名或注释清理。

## 7. Qt 使用规则

- Qt 只属于 UI 层或明确外层适配器。
- QWidget、QOpenGLWidget、QApplication、QString、QImage 等 Qt 类型不得进入 domain/application。
- application 不创建 QWidget/QOpenGLWidget，不返回 Qt 类型。
- Qt Designer 生成的 `.ui` 文件属于 UI 层。
- `QOpenGLWidget` 的 OpenGL 调用必须发生在有效 context 生命周期内。
- 原型阶段可以在 `QOpenGLWidget` 内直接实现绘制；稳定后再拆分资源和用例。
- `composition_root/qt_main.cpp` 只做 QApplication、窗口创建、依赖装配和事件循环。
- `composition_root/lesson_main.cpp` 只做课程选择、课程入口调用和进程返回码处理。

## 8. OpenGL、Shader、Texture 与资源规则

- 所有 `glXXX` 调用必须发生在有效 OpenGL Context 所在线程。
- Shader 编译、Program 链接、图片加载、纹理上传失败必须检查。
- OpenGL Program、Shader、Texture、VAO、VBO、EBO 必须有明确所有者和释放时机。
- 拥有 OpenGL ID 的类型最终应走 RAII，禁止无意复制。
- 修改 Shader attribute/uniform 时同步检查 C++ 端绑定。
- 纹理上传时根据通道数选择格式；必要时设置并恢复 `GL_UNPACK_ALIGNMENT`。
- CPU 像素数据上传后及时释放。
- 相机实时图像显示应优先使用：
  - 初始化时 `glTexImage2D` 分配纹理存储。
  - 每帧用 `glTexSubImage2D` 更新内容。
  - 避免每帧重新创建 Texture、Shader、VAO/VBO/EBO。
- 当前已验证更稳定的图片显示路径是 `stb_image + 原生 OpenGL Texture`，不要无故切回 `QImage/QOpenGLTexture`。

## 9. third_party 规则

- `third_party/` 默认只读。
- 不修改 GLAD、GLFW、GLM、stb 源码来规避上层问题。
- 不在 third_party 中放项目业务逻辑。
- 不自行联网下载、升级或替换依赖，除非用户明确授权。
- 新增或更新第三方依赖前说明版本、许可证、平台、ABI、CMake target 和影响。

## 10. assets、shaders、textures 规则

- 教程资源按章节和课程组织：

```text
assets/shaders/<chapter>/<lesson>/
assets/textures/<chapter>/<lesson>/
assets/models/<chapter>/<lesson>/
```

- UI 原型资源可暂放：

```text
assets/textures/ui/
```

- 不硬编码用户本机绝对路径。
- 修改 Shader 时同步检查 attribute、uniform、纹理单元和输入输出。
- 新增纹理时验证格式、通道数、垂直翻转策略和加载失败信息。
- 不随意重新编码、压缩或替换现有二进制资源。

## 11. 文档同步规则

- 架构层次、依赖方向、target、构建命令或平台支持变化时，同步更新 `DOC/ARCHITECTURE.md` 和必要的 `DOC/README.md`。
- 编码、命名、资源所有权、Qt/OpenGL 约定或 CMake 规则变化时，同步更新 `DOC/CODING_STYLE.md`。
- 新增课程方式、功能模块、相机/点云扩展、迁移策略变化时，同步更新 `DOC/EXTENDING.md`。
- 长期目标、阶段性决策、最近重要上下文变化时，同步更新 `.agents/CODEX_CONTEXT.md`。
- 文档必须区分当前实现、临时原型和目标架构。

## 12. 修改前计划要求

以下任务修改前必须给出简短计划：

- 多文件修改。
- 跨层依赖变化。
- CMake target 或依赖变化。
- 公共接口变化。
- Qt/OpenGL Context 生命周期变化。
- 相机 SDK、点云、轨迹等功能模块扩展。

小型单文件修复可以用一句话说明修改范围，但仍需先检查相关文件和边界。

## 13. 构建、测试和运行

执行命令前先读取当前 `CMakePresets.json`，以文件中的 preset 为准。

当前 Windows PowerShell 推荐构建入口：

```powershell
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Debug -NoPause
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Release -NoPause
```

已初始化 MSVC x64 环境时可直接使用：

```powershell
cmake --preset ninja-msvc-debug
cmake --build --preset ninja-msvc-debug

cmake --preset ninja-msvc-release
cmake --build --preset ninja-msvc-release
```

Debug 运行：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Qt.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe --list
```

验证要求：

- 修改 C++、CMake、公共头文件或 target 关系后，至少完成 Debug configure/build。
- 修改 Shader、纹理、窗口或渲染行为后，应在支持 GUI/OpenGL 的环境中运行验证；无法运行时明确说明。
- 当前没有自动化测试配置，不得声称运行了 `ctest`。
- 验证结束后运行 `git status --short` 和 `git diff --check`。

## 14. 明确禁止事项

- 禁止让 domain 依赖 Qt、OpenGL、GLAD、GLFW、GLM、stb_image、相机 SDK 或外层模块。
- 禁止让 application 创建 QWidget/QOpenGLWidget、调用 `glXXX` 或依赖具体 UI/基础设施。
- 禁止在 `main.cpp` 堆积业务逻辑、渲染实现、相机采集或资源加载。
- 禁止修改 third_party，除非用户明确授权。
- 禁止硬编码本机绝对路径、Qt 安装路径或 Visual Studio 安装路径。
- 禁止新增与当前任务无关的依赖、包管理器、工具链、preset 或代码生成器。
- 禁止添加“以后可能会用”但当前没有调用点、没有业务价值的宏、接口、配置项、包装层或抽象。
- 禁止用全局 include/link 配置掩盖模块边界错误。
- 禁止未经请求进行全工程重构、批量重命名或格式化。
- 禁止覆盖、删除、还原用户已有改动。
- 禁止提交 `out/`、IDE 缓存、临时文件或本地构建产物。
- 禁止伪造构建、测试或运行结果。
- 禁止把临时 UI 原型描述成最终架构。
