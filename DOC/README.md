# LearnOpenGLCN 工程文档

本目录记录 LearnOpenGLCN 的代码架构、模块边界、编码约定和扩展方式。

工程正在从教程示例集合逐步演进为采用整洁架构（Clean Architecture）的 OpenGL 学习项目。文档会明确区分“当前实现”和“目标架构”：当前代码尚未具备的能力不会被描述成已经完成。

## 文档索引

- [代码架构与依赖关系](./ARCHITECTURE.md)：当前结构、目标分层、依赖规则及各模块职责。
- [编码规范](./CODING_STYLE.md)：C++、OpenGL、CMake、资源和注释约定。
- [扩展指南](./EXTENDING.md)：新增课程、基础设施适配器和架构演进的推荐步骤。

## 当前工程状态

当前工程使用 CMake 3.21+、C++17、Ninja 和 MSVC 构建，主要依赖 OpenGL、GLFW、GLAD、GLM 与 stb_image。

现有主要构建链路为：

```text
LearnOpenGLCN (executable)
    └── lessons (static library)
        ├── infrastructure (static library)
        │   ├── glad
        │   ├── glfw3
        │   ├── OpenGL::GL
        │   └── glm::glm
        └── stb_image
```

`domain/`、`application/` 和 `ui/` 已建立目录，但尚未形成对应 CMake target。它们是后续整洁架构迁移的目标位置，不代表当前已经完成分层。

## 构建入口

推荐在 Windows PowerShell 中使用仓库根目录下的脚本初始化 MSVC 环境并构建：

```powershell
.\msvc-cmake.ps1 -Config Debug -NoPause
```

Release 构建：

```powershell
.\msvc-cmake.ps1 -Config Release -NoPause
```

构建产物默认位于：

```text
out/build/<preset>/bin/LearnOpenGLCN.exe
```

