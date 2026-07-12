# LearnOpenGLCN 工程文档

本目录记录 LearnOpenGLCN 的代码架构、模块边界、编码约定和扩展方式。

工程的长期目标是系统学习并整理 LearnOpenGLCN 网站中的全部内容，同时把部分学习分支扩展成可复用功能模块，例如使用 Qt + OpenGL 显示工业相机图像、用 OpenGL 绘制三维轨迹或点云画面。

工程正在从教程示例集合逐步演进为采用整洁架构（Clean Architecture）的 OpenGL 学习与功能实验项目。文档会明确区分“当前实现”和“目标架构”：当前代码尚未具备的能力不会被描述成已经完成。前期允许在 `ui` 层先跑通 Qt/OpenGL 原型，随后再按 domain/application/infrastructure/ui/app 分层拆分。

## 文档索引

- [代码架构与依赖关系](./ARCHITECTURE.md)：当前结构、目标分层、依赖规则及各模块职责。
- [编码规范](./CODING_STYLE.md)：C++、OpenGL、CMake、资源和注释约定。
- [扩展指南](./EXTENDING.md)：新增课程、基础设施适配器和架构演进的推荐步骤。
- [二维阿基米德螺旋轨迹](./TRAJECTORY_2D.md)：当前二维轨迹模型、分段生成、点距求解、导出和后续扩展边界。
- [Codex 上下文](../.agents/CODEX_CONTEXT.md)：记录当前长期目标、阶段性决策和最近上下文，避免后续对话过长后丢失背景。

## 当前工程状态

当前工程使用 CMake 3.21+、C++17、Ninja 和 MSVC 构建，主要依赖 OpenGL、GLFW、GLAD、GLM、stb_image、Qt Widgets/OpenGLWidgets，并已为大恒 Galaxy SDK 建立第三方 CMake target。

现有主要构建链路为：

```text
LearnOpenGLCN_Lessons (executable)
    ├── lessons (static library)
    │   ├── infrastructure (static library)
    │   └── stb_image
    └── Qt6::Widgets

lessons (static library)
    └── infrastructure (static library)
        ├── learnopengl_application
        ├── glad
        ├── glfw3
        ├── OpenGL::GL
        └── glm::glm

learnopengl_application (static library)
    └── learnopengl_domain (static library)

LearnOpenGLCN_Qt (executable)
    ├── learnopengl_ui (static library)
    └── infrastructure (static library)

learnopengl_ui (static library)
    ├── learnopengl_application
    ├── learnopengl_domain
    ├── Qt6::Widgets
    ├── Qt6::OpenGLWidgets
    ├── Qt6::Concurrent
    ├── Qt6::OpenGL
    ├── glad
    ├── OpenGL::GL
    └── stb_image

infrastructure (static library)
    ├── learnopengl_application
    ├── Galaxy::SDK
    ├── glad
    ├── glfw3
    ├── OpenGL::GL
    └── glm::glm
```

`domain/` 与 `application/` 已形成相机预览所需的最小 target：`learnopengl_domain` 提供 `VideoFrame` / `PixelFormat` 等稳定模型，并包含 `domain/trajectory` 中的二维阿基米德螺旋点生成纯算法；`learnopengl_application` 提供 `ICameraDevice` 和 `CameraPreviewService`。`infrastructure/camera/galaxy` 提供大恒相机适配器，`ui/` 承载当前 Qt/OpenGL 显示原型。

当前 Qt 主窗口还包含“轨迹算法 / 螺旋线导出”页面：它使用 domain 中的二维算法生成多段轨迹，并在后台导出合并 txt 和分段 txt。轨迹公式、分段规则、残差二分求解和边界点行为见 [二维阿基米德螺旋轨迹](./TRAJECTORY_2D.md)。

`LearnOpenGLCN_Lessons.exe` 不带参数会打开 Qt 课程导航器；课程列表来自 `lessons/catalog` 的 `LessonRegistry`，导航窗口实现位于 `composition_root/lesson_launcher`。传入课程 ID 时仍可直接运行对应 GLFW 课程，例如 `LearnOpenGLCN_Lessons.exe transform`。

当前 `third_party/Galaxy` 已包装出 `Galaxy::SDK`。现有目录包含大恒 VC/C API 与 C++ SDK 的头文件和 import library，可支持 `GxIAPI`、`DxImageProc`、`GalaxyIncludes.h`、`IGXFactory`、`CGXDevicePointer` 等接口。大恒运行时 DLL 直接放在 `out/build/<preset>/bin`，和 exe 一起提交运行。

## 构建入口

推荐在 Windows PowerShell 中使用仓库根目录下的脚本初始化 MSVC 环境并构建：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
```

Release 构建：

```powershell
.\msvc-cmake.ps1 -Config Release -NoPause
```

在 VS Code 中排查堆越界、释放后访问等内存错误时，可以通过 CMake Tools 选择 `ninja-msvc-asan` preset。该配置使用 MSVC AddressSanitizer，并与普通 Debug/Release 构建目录隔离：

```text
CMake: Select Configure Preset -> ninja-msvc-asan
CMake: Build
Run and Debug -> LearnOpenGLCN Qt - ASan
```

ASan 构建产物位于 `out/build/ninja-msvc-asan/bin`。构建过程会把所需的 MSVC ASan 运行时 DLL 自动部署到该目录，不要求每次手动初始化 PowerShell 运行环境。

构建产物默认位于：

```text
out/build/<preset>/bin/LearnOpenGLCN_Qt.exe
out/build/<preset>/bin/LearnOpenGLCN_Lessons.exe
```

Debug 构建后运行 Qt 原型：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Qt.exe
```

Debug 构建后运行课程代码：

```powershell
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe transform
.\out\build\ninja-msvc-debug\bin\LearnOpenGLCN_Lessons.exe --list
```
