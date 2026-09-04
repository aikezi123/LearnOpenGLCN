# EngineeringLab 项目协作规则

本文件适用于仓库根目录及所有子目录，只记录 Agent 在本项目中分析、修改和验证工作时必须遵守的执行规则。正式架构和模块知识统一维护在 `DOC/`。

## 1. 开始工作前

按任务范围读取：

1. 根目录 `AGENTS.md`
2. `.agents/AGENTS.md`
3. `.agents/CODEX_CONTEXT.md`
4. `DOC/README.md`
5. 与任务相关的架构、模块或开发指南
6. 顶层及相关模块的 `CMakeLists.txt`、公共头文件和实现文件

同时必须：

- 先运行 `git status --short`，识别并保留用户已有改动。
- 使用 `rg` 搜索现有实现、命名和调用点，避免重复抽象。
- 采用完成当前需求所需的最小修改，不增加没有真实调用点的预留接口、宏、开关或包装层。
- 多文件、跨层、CMake target、公共接口或生命周期变更前给出简短计划。
- 不对无关文件做格式化、重命名、注释清理或顺手重构。

如果文档与代码或 CMake 不一致，以当前实现为事实依据，并在本次任务范围内同步修正文档。

## 2. 架构边界

长期依赖方向必须指向内层：

```text
composition_root -> ui
composition_root -> infrastructure
composition_root -> application
ui             -> application -> domain
infrastructure -> application -> domain
```

各层职责：

- `domain`：稳定模型、值对象和纯算法；不得依赖 Qt、OpenGL、GLAD、GLFW、GLM、stb_image、相机 SDK 或外层模块。
- `application`：用例、流程和外部能力端口；可依赖 domain，不得创建 Qt 对象、调用 `glXXX` 或认识具体 infrastructure/ui 类型。
- `infrastructure`：实现 application 端口并封装 OpenGL、文件系统、并发工具和厂商 SDK 等技术细节；第三方类型不得泄漏到内层公共接口。
- `ui`：Qt 窗口、控件、事件、跨线程界面切换和可视化交互；业务规则应逐步下沉，不得让 Qt 类型进入 domain/application。
- `composition_root`：选择具体实现、创建对象并注入依赖，只承载启动与装配。
- `lessons`：保留可运行、可对照的 LearnOpenGL 教程示例；不要为了单个课程无关地重写其他课程。

外层类继承 application 接口叫“实现端口”；组合根创建具体对象并以端口类型传给 service 叫“依赖注入”。不要把具体实现或对象工厂提前放进 application。

原型阶段允许在 `QOpenGLWidget` 内先跑通 Qt/OpenGL 显示闭环，但必须在文档中明确它是当前原型，而不是最终资源边界。

## 3. C++、Qt 与 OpenGL

- 使用 C++17 和 UTF-8。
- 类型、枚举用 PascalCase；函数和局部变量用 camelCase；成员变量用 `m_` + camelCase。
- 头文件使用唯一 include guard 或 `#pragma once`，禁止 `using namespace`。
- include 顺序为：对应头文件、项目头文件、第三方头文件、标准库头文件。
- 明确对象所有权、线程归属、回调注销和析构顺序；重型 SDK 适配器优先用 Pimpl 隐藏实现。
- Qt 只属于 UI 或明确的外层适配器。`QWidget`、`QOpenGLWidget`、`QString`、`QImage` 等类型不得进入 domain/application。
- 所有 `glXXX` 调用以及 GPU 对象创建、使用和销毁必须发生在有效 OpenGL Context 所在线程。
- OpenGL Program、Shader、Texture、VAO、VBO、EBO 必须有明确所有者和释放时机；拥有 OpenGL ID 的类型最终应使用 RAII 且禁止无意复制。
- Shader 编译、Program 链接、图片加载和纹理上传失败必须检查；修改 attribute、uniform、纹理格式或纹理单元时同步检查 C++ 绑定。
- 相机实时纹理应初始化时用 `glTexImage2D` 分配存储、每帧用 `glTexSubImage2D` 更新，避免逐帧重建 GPU 资源。该决策的背景记录在 `DOC/modules/CAMERA_ARCHITECTURE.md`。

## 4. CMake、目录与资源

- 保持 CMake 3.21+ 和 C++17，除非用户明确要求升级。
- 使用 target-based CMake；不要新增全局 `include_directories()`、`link_libraries()` 或无关全局宏。
- 分层代码采用“层 / 模块 / include + src”的模块优先结构。公开 include 目录不要重复嵌套项目名或当前层名。
- 只有公共头文件所需依赖才使用 `PUBLIC`，实现细节使用 `PRIVATE`。
- domain/application target 不得链接 Qt、OpenGL、GLAD、GLFW、stb_image 或相机 SDK。
- 修改 target 关系时同步检查顶层 `add_subdirectory()` 顺序和所有消费者。
- 不硬编码用户机器、Visual Studio、Qt 或资源的绝对路径。
- `assets/` 按章节、课程或明确模块组织；不要无故重新编码或替换现有二进制资源。
- `third_party/` 默认只读。除非用户明确授权，不修改、下载、升级或替换第三方依赖；确需变更时说明版本、许可证、平台、ABI、CMake target 和影响。
- 不提交 `out/`、IDE 缓存、临时文件或本机构建产物。

## 5. 文档维护

`DOC/README.md` 是正式文档入口。新增文档先按职责归类：

- 总体结构和依赖：`DOC/architecture/`
- 具体功能设计、阶段与边界：`DOC/modules/`
- 编码、扩展、构建和测试方法：`DOC/guides/`

同步原则：

- 架构、依赖方向、target、构建入口变化时更新架构文档和必要的 README。
- 模块行为、生命周期、阶段进度或已知边界变化时更新对应模块文档。
- 编码、CMake、Qt/OpenGL 或资源约定变化时更新相应指南。
- 只把跨对话继续工作需要的当前快照写入 `.agents/CODEX_CONTEXT.md`，不要复制完整设计说明、调试日志或 TODO 清单。
- 文档必须区分“当前实现”“已验证结论”“未来候选”，不得把计划描述成已完成。

## 6. 构建与验证

执行前读取当前 `CMakePresets.json`，以仓库实际 preset 为准。Windows PowerShell 推荐入口：

```powershell
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Debug -NoPause
powershell -ExecutionPolicy Bypass -File .\msvc-cmake.ps1 -Config Release -NoPause
```

验证要求：

- 修改 C++、公共头文件、CMake 或 target 关系后，至少完成 Debug configure/build。
- 修改已有测试覆盖的行为后运行对应 CTest；报告实际发现、执行和通过的用例数量，不把占位测试描述为行为覆盖。
- 修改 Shader、纹理、窗口、相机或渲染行为后，在条件允许时运行 GUI/OpenGL 验证；无法运行时明确说明。
- 纯文档调整至少检查 Markdown 相对链接、过期引用、`git diff --check` 和 `git status --short`。
- 完成后说明实际修改、实际验证和仍未验证的部分，禁止伪造构建、测试或运行结果。

## 7. 禁止事项

- 不覆盖、删除或还原用户已有修改。
- 不运行破坏性 Git 命令，不未经请求提交或推送。
- 不修改 `.git/` 或生成文件来掩盖源代码问题。
- 不把业务逻辑、渲染实现、相机采集或资源加载堆入 `main.cpp`。
- 不用全局 include/link 配置掩盖模块边界错误。
- 不新增与当前任务无关的依赖、包管理器、工具链、preset、代码生成器或抽象。
- 不把临时 UI 原型、计划事项或未经运行验证的行为描述为正式完成状态。
