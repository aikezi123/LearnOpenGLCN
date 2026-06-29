# 编码规范

## 1. 基本原则

- 代码首先服务于正确性和可读性，其次才是减少行数。
- 教程代码可以保留必要的解释和步骤，但注释应解释“为什么”，避免逐字复述代码。
- 新代码遵守模块边界，不通过增加 include path 绕过依赖问题。
- GPU、窗口、文件等资源必须体现所有权和生命周期。
- 优先完成一个清晰的小抽象，不提前设计尚未出现的扩展点。
- 当前 Qt/OpenGL 功能原型允许先在 `ui` 层跑通完整流程，但应明确标记为阶段性实现；稳定后再迁移到 application/infrastructure/domain 的合适位置。

## 2. C++ 版本与文件

- 使用 C++17。
- 头文件使用 `.h` 或 `.hpp` 时，在同一模块内保持一致；基础设施中的 C++ 类推荐 `.hpp`。
- 实现文件使用 `.cpp`。
- 文件统一使用 UTF-8 编码。
- 每个头文件使用 `#pragma once` 或唯一 include guard，不重复使用过于宽泛的宏名。
- 一个主要类型对应一组同名头文件和实现文件。

## 3. 命名规则

推荐约定：

| 对象 | 规则 | 示例 |
| --- | --- | --- |
| 命名空间 | 小写 | `learnopengl::infrastructure` |
| 类型、枚举 | PascalCase | `ShaderProgram` |
| 函数、局部变量 | camelCase | `loadTexture()`、`vertexCount` |
| 成员变量 | `m_` + camelCase | `m_programId` |
| 常量 | `k` + PascalCase | `kDefaultWidth` |
| CMake target | 小写下划线 | `learnopengl_application` |
| CMake target alias | 命名空间形式 | `learnopengl::application` |
| Shader uniform | camelCase | `modelMatrix` |

课程入口应统一命名，例如 `runCoordinateTransformationLesson()`。避免继续引入 `transform()` 这类含义过宽的全局函数。

文件名需要表达内容并保证拼写正确。已有 `textrues` 等历史名称可以在迁移对应课程时修复，不要求无关改动顺手大面积重命名。

## 4. 命名空间与头文件

项目代码放在 `learnopengl` 根命名空间下，并按层和功能继续划分：

```cpp
namespace learnopengl::infrastructure::opengl {
    class ShaderProgram final {
        // ...
    };
}
```

include 应从模块公开根目录开始：

```cpp
#include <learnopengl/infrastructure/opengl/ShaderProgram.hpp>
```

禁止依赖偶然被加入搜索路径的短 include：

```cpp
// 不推荐
#include "Shader.hpp"
```

头文件规则：

- 头文件必须能够独立编译，直接包含自己使用的类型声明。
- 优先前置声明，只有需要完整定义时才 include。
- 不在头文件使用 `using namespace`。
- application/domain 的公共头文件不得 include GLFW、GLAD、stb_image。
- include 顺序建议为：对应头文件、项目头文件、第三方头文件、标准库头文件；各组之间空一行。

## 5. 类型、函数和接口

- 使用 `nullptr`，不使用 `NULL` 或整数 `0` 表示空指针。
- 不修改对象的成员函数标记为 `const`。
- 单参数构造函数根据语义使用 `explicit`。
- 接口析构函数使用 `virtual ~Interface() = default`。
- 不需要继承的实现类可以标记 `final`。
- 函数保持单一职责；课程入口负责流程编排，资源类负责资源生命周期。
- 使用强类型枚举 `enum class`。
- 对尺寸和索引选择与 API 匹配的类型，跨边界时进行明确转换。
- 不暴露可修改的原始 OpenGL ID；必要时只提供受控的只读访问。

## 6. 所有权与 RAII

OpenGL 资源类应遵守 RAII：构造或工厂函数创建资源，析构函数释放资源。

拥有 OpenGL ID 的类型：

- 禁止复制。
- 支持移动，或显式禁止移动。
- 析构前必须保证对应 OpenGL Context 仍然有效。
- 创建失败时不能留下“看似有效”的半初始化对象。

示意：

```cpp
class ShaderProgram final {
public:
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    ~ShaderProgram();

private:
    unsigned int m_programId{0};
};
```

纹理、VAO、VBO、EBO 和 Program 均应逐步采用相同原则。

## 7. 错误处理

- 检查 `glfwInit()`、窗口创建、GLAD 初始化、文件读取和图片解码结果。
- 错误信息必须包含操作、资源路径或对象类型等定位上下文。
- infrastructure 将第三方错误转换成项目可理解的错误，不让 application 解析第三方错误码。
- 构造失败时优先使用异常、结果类型或工厂函数，避免仅打印日志后继续使用无效对象。
- 不在底层库中无条件终止整个进程；进程退出策略由 app 决定。
- 调试构建可增加 OpenGL Debug Callback，但不替代显式状态检查。

## 8. OpenGL 约定

- 所有 OpenGL 调用必须发生在有效 Context 所在线程。
- 初始化 GLAD 后才能调用 OpenGL 函数。
- 对隐式绑定状态保持谨慎；函数应说明自己要求或修改的绑定状态。
- attribute location 优先在 GLSL 中显式声明。
- uniform 名称集中定义或封装在 Shader 接口中，避免散落字符串。
- 变换矩阵明确记录乘法顺序。对于 `T * R * S * position`，顶点实际依次经历缩放、旋转和平移。
- 上传纹理时根据通道数选择正确格式，并考虑 `GL_UNPACK_ALIGNMENT`。
- 退出 Context 前释放依赖 Context 的 GPU 资源。
- 在 `QOpenGLWidget` 中创建和释放 OpenGL 资源时，必须确认当前对象的 context 有效；析构中需要 `makeCurrent()` 后再删除 GL 资源，随后 `doneCurrent()`。
- 用于相机图像或实时帧更新的纹理，优先采用“初始化时 `glTexImage2D` 分配存储，每帧 `glTexSubImage2D` 更新内容”的方式，避免每帧重新创建纹理对象。

## 8.1 Qt 使用约定

- Qt 类型只允许出现在 UI 层或明确的外层适配器中。
- `QWidget`、`QOpenGLWidget`、Qt Designer `.ui`、信号槽和 Qt 事件循环不得进入 domain/application。
- application 不返回 `QImage`、`QString`、`QWidget`、`QOpenGLWidget` 等 Qt 类型。
- `composition_root/qt_main.cpp` 只负责创建 `QApplication`、设置必要的 OpenGL 格式、装配顶层窗口并进入事件循环。
- `composition_root/lesson_main.cpp` 只负责选择和调用课程入口，不放课程内部渲染逻辑。
- 原型阶段可以让 `QOpenGLWidget` 直接管理 Shader、VAO/VBO/EBO、Texture；当逻辑稳定或被多处复用时，应抽到 infrastructure 的 OpenGL 资源封装中。
- 不同时运行 GLFW 主循环和 Qt 主事件循环。迁移 GLFW 示例到 Qt 时，应明确哪个系统拥有窗口、事件循环和 OpenGL Context。

## 9. 资源与 Shader

- 资源路径使用 `/` 作为逻辑分隔，实际定位交给资源服务或 `std::filesystem`。
- 不在 domain/application 中拼接源码根目录绝对路径。
- 每个 Shader 文件只承担明确阶段职责。
- GLSL 版本与创建的 OpenGL Context 版本保持一致。
- Shader 的 attribute、uniform、输入输出变量是代码契约，修改时同步更新 C++ 调用方。
- 图片加载后及时释放 CPU 侧像素数据；GPU Texture 由 RAII 对象负责释放。
- 使用 stb_image 加载纹理时，明确是否调用 `stbi_set_flip_vertically_on_load()`，并保持纹理坐标与图像翻转策略一致。
- 若强制加载为 RGBA，例如 `STBI_rgb_alpha`，上传格式应与之匹配为 `GL_RGBA`/`GL_RGBA8`。

## 10. CMake 规范

- 使用 target-based CMake，不使用目录级全局配置传播依赖。
- 每个模块通过 `target_sources()`、`target_include_directories()` 和 `target_link_libraries()` 描述自身。
- `PRIVATE`、`PUBLIC`、`INTERFACE` 必须反映真实使用需求。
- 公共 include 路径只暴露模块的 `include` 根目录。
- 第三方依赖尽可能包装为稳定 target，不在多个模块重复硬编码库路径。
- 新增 target 后提供 `learnopengl::name` 别名。
- 平台相关链接放在对应适配器或 third_party target 中。
- 不把生成文件写入源码目录，统一放在 `out/`。

## 11. 注释与文档

- 注释使用完整、简洁的中文或英文，同一段落不要无意义混杂。
- 复杂数学、OpenGL 状态要求和架构决策需要说明原因。
- 删除已经失效的注释，不用注释保存旧代码。
- 公共接口说明输入、输出、所有权、失败方式和线程/Context 要求。
- 架构边界发生变化时同步更新 `DOC/`。

## 12. 提交前检查

- Debug 配置能够从干净构建目录完成配置和编译。
- 新增资源可在非源码工作目录下被定位。
- 没有把 `out/` 或本地 IDE 文件加入版本控制。
- 没有新增跨层反向依赖。
- GPU 和系统资源在所有返回路径上都能正确释放。
- 新增公开接口具有最小必要文档。
