# LearnOpenGLCN 项目协作规则

本文件适用于仓库根目录及所有子目录。Codex 在本项目中分析、修改、构建或验证代码时必须遵守这些规则。用户当前请求与本文件冲突时，先指出冲突并请求确认；不要静默突破架构边界或扩大修改范围。

## 1. 项目背景与当前状态

LearnOpenGLCN 是一个使用 C++17 学习和整理 LearnOpenGL 教程的 Windows OpenGL 工程，使用 CMake 3.21+、Ninja 和 MSVC 构建。当前第三方依赖为 GLAD、GLFW、GLM 和 stb_image，并链接系统 OpenGL。

当前真实 target 关系是：

```text
LearnOpenGLCN (executable)
    └── lessons (STATIC)
        ├── infrastructure (STATIC)
        │   ├── glad (STATIC)
        │   ├── glfw3 (IMPORTED STATIC)
        │   ├── OpenGL::GL
        │   └── glm::glm (INTERFACE)
        └── stb_image (STATIC，另有 stb::image alias)
```

当前顶层 CMake 只加入 `third_party`、`infrastructure`、`lessons` 和 `app`。`domain/`、`application/`、`ui/` 已有目录，但尚无对应 CMake target；它们是目标整洁架构的一部分，不得把目标状态描述成已经实现。

当前 `app/main.cpp` 直接选择并调用一个 lesson 入口，所有 lesson 源码仍被编入单一 `lessons` 静态库。保留这一事实，除非用户明确要求进行课程模块化迁移。

Qt 当前未被 CMake 或源码引入。Qt 规则是未来接入约束，不代表允许 Codex自行新增 Qt 依赖。

## 2. 修改前必须阅读和检查

开始修改前，按任务相关性优先阅读：

1. 本文件 `AGENTS.md`。
2. `DOC/README.md`。
3. `DOC/ARCHITECTURE.md`。
4. `DOC/CODING_STYLE.md`。
5. `DOC/EXTENDING.md`。
6. 根目录 `CMakeLists.txt` 和 `CMakePresets.json`。
7. 被修改模块及其直接依赖模块的 `CMakeLists.txt`、公开头文件和实现文件。
8. 若涉及构建环境，再读取 `msvc-cmake.ps1`；不得仅凭记忆猜测脚本参数。
9. 若涉及 Shader、纹理或模型，同时读取调用它们的 C++ 代码和对应 `assets/` 文件。

修改前还必须：

- 运行 `git status --short`，识别用户已有改动；不得覆盖、还原或格式化无关改动。
- 确认任务属于哪一层、会改变哪些 target、是否新增跨层依赖。
- 搜索现有同类实现和命名，避免创建重复抽象。
- 明确资源所有者、创建时机、销毁时机以及 OpenGL Context 生命周期。
- 对多文件、跨层或 CMake 变更先给出简短计划；计划列出修改范围、依赖方向和验证方式。
- 小型单文件修复可以使用简短的一步计划，但仍要先检查相关边界。
- 如果需求会改变公共接口、架构方向、第三方依赖或平台支持，先说明影响；缺少必要决策时向用户确认，不擅自选择。

## 3. 整洁架构总规则

长期依赖方向必须指向内层：

```text
app ─┬─> ui ──────────────┐
     ├─> infrastructure ──┼─> application ─> domain
     └─> lessons ─────────┘
```

必须遵守：

- `domain` 不知道任何外层模块或外部图形/UI 技术。
- `application` 只依赖 `domain` 和 C++ 标准库，并在内层定义所需端口。
- `infrastructure`、`ui` 和外层 lesson 实现 application 定义的端口。
- `app` 是 Composition Root，负责创建具体实现和连接依赖。
- 第三方库和操作系统类型不得泄漏到 `domain` 或 `application`。
- 依赖倒置通过小而明确的接口完成，不创建包罗万象的 `Manager`、`Service` 或巨型包装接口。
- 当前教程型 lesson 可以直接演示 OpenGL；不要为了形式上的纯洁隐藏课程正在教授的 API。
- 架构迁移采用一条可运行的纵向切片逐步推进，不一次性重写全部 lesson。

## 4. 各层职责和禁止事项

### 4.1 domain

允许：

- 表达与图形 API 无关的颜色、变换、网格描述、场景数据等稳定模型。
- 实现纯计算和领域规则。
- 依赖 C++ 标准库和 `domain` 内部代码。

禁止：

- 依赖 Qt、OpenGL、GLAD、GLFW、GLM、stb_image 或文件系统适配器。
- 包含 `QWidget`、`QOpenGLWidget`、`QOpenGLContext`、GLFW 窗口、OpenGL ID、`glXXX` 调用或第三方错误码。
- 依赖 `application`、`infrastructure`、`ui`、`lessons` 或 `app`。
- 为填充目录而创建没有业务含义的实体或接口。

### 4.2 application

允许：

- 编排用例、课程运行协议和程序流程。
- 定义最小端口，如渲染、窗口、输入、图片加载和时钟接口。
- 使用 domain 类型描述输入、输出和状态。
- 依赖 `domain` 和 C++ 标准库。

禁止：

- 创建 `QWidget`、`QOpenGLWidget` 或其他 Qt UI 对象。
- 调用 `glXXX`、GLFW、stb_image 或 Qt API。
- include GLAD、GLFW、Qt、stb_image 的头文件。
- 知道 infrastructure/ui 的具体类或硬编码 assets 的源码绝对路径。
- 决定底层进程终止、窗口实现或 GPU 资源 ID。

### 4.3 infrastructure

允许：

- 实现 application 定义的外部技术端口。
- 封装 OpenGL Shader、Program、Buffer、VertexArray、Texture 等资源。
- 使用 stb_image 读取图片，使用文件系统定位资源。
- 依赖 application/domain 和完成实现所需的第三方 target。
- 将第三方错误转换成项目可理解的错误。

禁止：

- 把第三方类型、宏和可修改的原始 OpenGL ID泄漏到 application/domain 公共接口。
- 在 infrastructure 决定运行哪一课程、组织 UI 页面或放置业务用例。
- 在没有有效 OpenGL Context 时创建、使用或销毁 GPU 对象。
- 仅打印错误后继续使用无效或半初始化资源。

### 4.4 ui

允许：

- 管理窗口、平台事件、键盘鼠标输入和可视化交互。
- 使用 GLFW；未来用户明确要求接入 Qt 后，可在此层使用 Qt Widgets、`QOpenGLWidget` 等 UI 类型。
- 将 UI/平台事件转换为 application 端口理解的输入。
- 调用 application 用例。

禁止：

- 在 UI 类中实现领域规则或长期保存 application 内部业务状态。
- 让 Qt/GLFW 类型进入 domain。
- 让 application 反向依赖 QWidget、QOpenGLWidget 或具体窗口类。
- 绕开 application，直接把所有课程编排和业务流程堆进事件回调。

### 4.5 app

允许：

- 作为唯一 Composition Root 创建 ui/infrastructure/lesson/application 对象并注入依赖。
- 解析启动参数、选择课程并控制进程级启动和退出。
- 保持少量、清晰的装配代码。

禁止：

- 在 `main.cpp` 中塞入领域规则、渲染算法、Shader 编译、纹理加载或大段窗口事件处理。
- 把 `main.cpp` 当作通用工具文件或共享状态容器。
- 让其他层依赖 `app`。

### 4.6 lessons

允许：

- 按 LearnOpenGL 章节组织可运行示例和练习。
- 教学型课程直接展示 OpenGL 调用；抽象型课程只使用 application 端口。
- 在当前迁移阶段保留独立完整的初始化和渲染步骤，以维持教学可读性。

禁止：

- 把可复用的资源所有权逻辑长期复制到越来越多课程；出现稳定重复后应提出小步抽取方案。
- 为修改一个课程顺手重写所有旧课程。
- 新增含义过宽的全局入口名；新入口使用明确名称和 `learnopengl` 命名空间。

## 5. CMake 修改规则

- CMake 最低版本保持为 3.21，C++ 标准保持为 C++17，除非用户明确要求升级并接受影响。
- 使用 target-based CMake；不得新增全局 `include_directories()`、`link_libraries()` 或为单模块需求设置全局依赖。
- 一个架构模块对应明确 target。新增 target 时优先提供 `learnopengl::name` alias。
- `target_sources()`、`target_include_directories()`、`target_link_libraries()` 的 `PRIVATE`、`PUBLIC`、`INTERFACE` 必须反映真实传播需求。
- 只有公共头文件需要的 include 和 link usage requirement 才能设为 `PUBLIC`；实现依赖使用 `PRIVATE`。
- 每个 target 只公开稳定的 `include/` 根目录，不公开 `src/`、嵌套命名空间目录或其他模块目录来修复 include 错误。
- 新模块优先显式列出源文件。现有 `lessons` 和 `infrastructure` 使用 `GLOB_RECURSE` 是已知债务，不在无关任务中顺手全量改写。
- application/domain target 不得链接 Qt、GLAD、GLFW、OpenGL 或 stb_image。
- 平台库只放在对应 ui/infrastructure/third_party target；不得硬编码本机路径。
- 当前 GLFW target 指向仓库内 `third_party/glfw/lib-vc2022/glfw3.lib`，这是现有 Windows/MSVC 约束；不要擅自换包管理器或下载替代版本。
- `LEARNOPENGL_ASSET_DIR` 当前由 `lessons` 以 `PUBLIC` 编译宏提供，这是已记录的架构债务。除非任务是资源定位迁移，否则不要扩大其使用范围。
- 修改 target 关系时同步核对顶层 `add_subdirectory()` 顺序，确保被依赖 target 先定义。
- 生成文件只能进入 `out/`，不得写入源码目录。
- 不新增与任务无关的工具链、包管理器、生成器、preset 或外部依赖。

## 6. C++ 编码规则

- 使用 C++17 和 UTF-8。
- 项目代码放入 `learnopengl` 根命名空间，并按层/模块继续划分；避免新增全局函数和全局可变状态。
- 类型、枚举使用 PascalCase；函数和局部变量使用 camelCase；成员变量使用 `m_` + camelCase；常量使用 `k` + PascalCase。
- 头文件使用 `#pragma once` 或唯一 include guard，必须能独立编译。
- include 从模块公开根目录开始，例如 `<learnopengl/Shader.hpp>`；不得依赖偶然泄漏的短路径如 `"Shader.hpp"`。
- include 顺序为对应头文件、项目头文件、第三方头文件、标准库头文件，各组之间空行。
- 头文件中不得使用 `using namespace`；优先前置声明，但使用完整类型时直接包含所需头文件。
- 使用 `nullptr`、`enum class`、`const` 正确性和明确类型转换。
- 单参数构造函数按语义使用 `explicit`；接口提供虚析构函数；不需继承的实现类可标记 `final`。
- 函数保持单一职责，错误消息包含操作、资源路径或对象类型等定位信息。
- 不在底层库无条件结束进程；退出策略由 app 决定。
- 不对无关文件做批量格式化、拼写修正、重命名或注释清理。

## 7. Qt 使用规则

- 当前工程没有 Qt target、Qt CMake 配置或 Qt 源码。没有用户明确需求时，不新增 Qt、`find_package(Qt...)`、AUTOMOC/AUTOUIC/AUTORCC 或 Qt 相关 preset。
- 接入 Qt 前先读取当时的 `CMakePresets.json`、顶层及 ui/app CMake 配置；若命令或 Qt 版本无法从工程确定，不猜测，先向用户确认。
- QWidget、QOpenGLWidget、QWindow、信号槽和 Qt 事件循环属于 `ui` 外层。
- Qt 文件、资源和平台适配实现不得进入 domain/application。
- application 通过普通 C++ 端口接收输入和发出结果，不返回 QWidget、QString、QImage 等 Qt 类型。
- Qt 与 OpenGL Context 的桥接放在 ui 或明确的外层适配器中，必须写清 Context 所在线程和销毁顺序。
- `main.cpp` 只允许创建 `QApplication`、装配依赖、显示顶层窗口并进入事件循环；业务逻辑通过 application 用例执行。
- 不同时维护相互竞争的 GLFW 和 Qt 主事件循环；如确需迁移，先给出事件循环及 Context 所有权方案并获得用户确认。

## 8. OpenGL、Shader 与资源管理规则

- 所有 `glXXX` 调用必须发生在有效 OpenGL Context 所在线程，且在 GLAD 成功初始化之后。
- 检查 `glfwInit()`、窗口创建、GLAD 初始化、Shader 文件读取/编译/链接、图片解码等失败路径。
- OpenGL Program、Shader、Texture、VAO、VBO、EBO 等资源必须有明确所有者和释放时机。
- 新增资源封装优先采用 RAII：拥有 OpenGL ID 的类型禁止复制，明确移动语义，并在 Context 销毁前析构。
- 不公开可修改的 OpenGL ID；需要底层访问时提供范围受控的只读接口。
- 对 OpenGL 隐式状态保持明确：函数应说明需要和修改的绑定、纹理单元、深度/混合状态。
- attribute location 优先在 GLSL 显式声明；uniform/attribute 名称是 C++ 与 Shader 的契约，修改一端必须同步检查另一端。
- GLSL 版本必须与创建的 OpenGL Context 版本兼容。
- 矩阵代码应说明乘法顺序；例如 `T * R * S * position` 对顶点的实际顺序是缩放、旋转、平移。
- 纹理上传根据实际通道数选择格式，必要时设置 `GL_UNPACK_ALIGNMENT`；CPU 像素使用完后立即释放。
- Shader 编译或链接失败后不得继续把无效 Program 当作成功对象使用。

## 9. assets、shaders 与 textures 规则

- 资源按章节和课程组织：`assets/shaders/<chapter>/<lesson>/`、`assets/textures/<chapter>/<lesson>/`；未来模型使用 `assets/models/<chapter>/<lesson>/`。
- 使用小写、明确、稳定的目录和文件名；不要创建含义不明的重复资源。
- domain/application 不拼接源码根目录绝对路径，也不依赖当前工作目录。
- 当前课程通过 `LEARNOPENGL_ASSET_DIR` 定位资源；新改动不得硬编码用户本机路径。
- 若迁移为构建期资源部署，应由 CMake 集中处理，不在各 lesson 中散落复制命令。
- 修改 Shader 时同步核对 C++ attribute、uniform、纹理单元及输入输出定义。
- 新增纹理时验证格式、通道数、垂直翻转策略和加载失败信息。
- 不为代码任务随意重新编码、压缩或替换现有二进制资源。

## 10. third_party 规则

- `third_party/` 视为外部代码和导入配置，默认只读；除非用户明确要求更新某依赖，否则不得修改其中源码、头文件、预编译库或 CMakeLists.txt。
- 不在 third_party 中加入项目业务逻辑、修复项目 include 边界或存放项目公共工具。
- 不直接编辑 GLAD 生成代码、GLFW/GLM/stb 源码来规避上层问题。
- 不自行联网下载、升级或替换依赖，不新增包管理器。
- 若必须更新依赖，先说明版本、许可证、平台、ABI、CMake target 和现有调用方影响，获得用户确认后再执行。

## 11. 文档同步规则

- 架构层次、依赖方向、target、构建命令或平台支持变化时，同步更新 `DOC/ARCHITECTURE.md` 和必要的 `DOC/README.md`。
- 编码、命名、资源所有权或 CMake 约定变化时，同步更新 `DOC/CODING_STYLE.md`。
- 新增课程方式、适配器流程、第三方接入或迁移阶段变化时，同步更新 `DOC/EXTENDING.md`。
- 文档必须区分“当前已实现”和“目标/建议”，不得把计划写成完成事实。
- 只修复实现且没有改变公共约定时，不为凑文档改动而编辑 DOC。
- 若用户限制只修改特定文件，优先遵守范围限制，并在结果中指出本应同步但未获授权的文档。

## 12. 构建、测试和运行

执行命令前先读取当前 `CMakePresets.json`，以文件中的 preset 为准；以下命令只对应当前配置。

当前推荐的 Windows PowerShell 构建入口会初始化 MSVC 环境：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
.\msvc-cmake.ps1 -Config Release -NoPause
```

在已初始化 MSVC x64 开发环境中，可直接使用当前 preset：

```powershell
cmake --preset ninja-msvc-debug
cmake --build --preset ninja-msvc-debug

cmake --preset ninja-msvc-release
cmake --build --preset ninja-msvc-release
```

当前可执行文件输出路径由 `app/CMakeLists.txt` 指定。Debug 构建后运行：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN.exe
```

Release 构建后运行：

```powershell
.\out\build\ninja-msvc-release\bin\LearnOpenGLCN.exe
```

验证要求：

- 修改 C++、CMake、公共头文件或 target 关系后，至少完成 Debug configure 和 build。
- 修改 Shader、纹理、窗口或渲染行为后，在支持图形窗口/OpenGL Context 的环境中运行受影响 lesson，检查启动、画面、窗口缩放、输入和退出；环境不支持时明确报告未做运行验证及原因。
- 修改纯文档时检查 Markdown 链接、代码块、`git diff --check` 和修改范围，不强制进行无关编译。
- 修改 Release 专有配置、优化相关代码或发布路径时，再执行 Release 构建。
- 当前工程没有 `testPresets`、`enable_testing()` 或 `add_test()`，不得声称已运行自动化测试，也不要猜测 `ctest` 命令。测试配置变化后先读取 `CMakePresets.json` 和相关 CMakeLists.txt 再确定命令。
- 构建失败时保留并报告第一个有意义的错误，区分源码错误、Shader/资源错误和未初始化 MSVC 环境；不要用扩大 include path 或修改 third_party 掩盖根因。
- 验证结束后运行 `git status --short` 和 `git diff --check`，确认没有生成文件或无关修改进入源码树。

## 13. 修改完成后的交付要求

- 简要说明实现结果，不只罗列执行步骤。
- 列出修改过的文件及其职责。
- 说明实际执行的 configure、build、test、run 或静态检查；没有执行的项目及原因也要明确。
- 指出仍存在的风险、已知架构债务或需要用户决策的后续事项。
- 不把构建成功等同于 OpenGL 视觉结果正确；Shader/渲染变更需要运行或明确说明未运行。

## 14. 明确禁止事项

- 禁止让 domain 依赖 Qt、OpenGL、GLAD、GLFW、GLM、stb_image 或外层模块。
- 禁止让 application 创建 QWidget/QOpenGLWidget、调用 `glXXX` 或直接依赖具体基础设施。
- 禁止在 `main.cpp` 堆积业务逻辑、渲染实现或资源加载。
- 禁止修改 third_party，除非用户明确授权依赖更新任务。
- 禁止硬编码用户本机绝对路径、Visual Studio 安装路径或源码根目录。
- 禁止新增与当前任务无关的依赖、包管理器、工具链、preset 或代码生成器。
- 禁止用全局 include/link 配置掩盖模块边界错误。
- 禁止未经请求进行全工程重构、批量重命名、格式化或架构迁移。
- 禁止覆盖、删除、还原用户已有改动；禁止使用 `git reset --hard`、`git checkout --` 等破坏性操作。
- 禁止提交 `out/`、IDE 缓存、临时文件或本地构建产物。
- 禁止伪造构建、测试或运行结果；无法验证时必须直说。
- 禁止把文档中的目标架构描述为当前已经落地。
