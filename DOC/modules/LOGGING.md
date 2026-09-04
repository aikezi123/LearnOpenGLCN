# 日志模块

## 1. 当前状态

工程已加入日志接口和 spdlog 基础设施实现。`EngineeringWorkbench` 在组合根创建一个进程级 `SpdlogLogger`，经 `AppComposition` 和 `CameraComposition` 按 `ILogger&` 注入 `CameraCaptureService`；服务内部已经持有绑定 `camera` 模块名的 `ModuleLogger`，但当前尚未调用任何日志方法。`OpenGLLessons`、其他 UI/业务模块和命令行工具仍未接入。

当前 target：

| Target | 类型 | 职责 |
| --- | --- | --- |
| `englab::diagnostics` | Interface | 定义日志数据、`ILogger` 端口和绑定模块名的 `ModuleLogger` |
| `englab::logging` | Static | 用 spdlog 实现异步/同步、滚动文件和可选控制台日志 |

依赖方向为：

```text
englab::diagnostics -> fmt::fmt provided by vcpkg (INTERFACE)
englab::application -> englab::diagnostics
englab::logging -> englab::diagnostics
englab::logging -> spdlog::spdlog provided by vcpkg (PRIVATE)
EngineeringWorkbench -> englab::logging
```

spdlog 类型通过 `SpdlogLogger` 的 Pimpl 隐藏，不进入 application 公共日志端口。`ModuleLogger` 在 C++17 下直接使用独立的 fmt 格式化库提供 `{}` 参数格式化；它不依赖 spdlog。domain 不依赖日志模块。

## 2. 第三方依赖

工程通过根目录 `vcpkg.json` 直接声明 fmt、spdlog 和 GoogleTest，版本由固定的 vcpkg `builtin-baseline` 决定。仓库不再保存 spdlog 的源码、头文件或预编译库；CMake 分别通过 `fmt::fmt` 和 `spdlog::spdlog` 标准 target 使用格式化能力与日志后端。

Windows preset 使用 `x64-windows-static-md` triplet，保持静态依赖和动态 MSVC CRT，并通过仅在 Windows 启用的 manifest feature 打开宽字符日志文件路径。未来 Linux、macOS 或其他架构使用对应 vcpkg triplet，不需要在仓库中增加平台二进制目录。首次缺少缓存时 vcpkg 会构建依赖，后续可复用本地或共享二进制缓存。

## 3. 已实现配置

`SpdlogLoggerOptions` 当前提供：

- logger 名称和日志文件路径。
- 最低日志级别。
- 单个滚动文件大小和保留文件数，默认 10 MiB、5 个文件。
- 同步或异步模式，默认异步。
- 异步队列容量，默认 8192，队列满时阻塞以避免静默丢失日志。
- 可选控制台 sink，默认关闭。

每条记录包含级别、组件名、消息及源码位置。日志输出格式包含毫秒时间、级别、线程 ID、logger 名称和源码行号。`ILogger::write()` 与 `flush()` 保证不把日志后端异常传播到调用方；构造阶段的无效配置或日志文件创建失败仍会报告异常，由未来的组合根决定降级策略。

`SpdlogLogger` 私有持有异步线程池。析构时先请求 flush，再销毁 logger 和线程池，确保队列排空并回收工作线程。

## 4. 头文件及使用方法

### 4.1 三个公共头文件的职责

底层日志端口位于：

```cpp
#include <diagnostics/ILogger.h>
```

该头文件提供项目自有的 `LogLevel`、`SourceLocation`、`LogRecord` 和 `ILogger`，不提供字符串格式化。需要直接提交完整 `LogRecord` 或实现测试替身时包含它。

业务组件通常包含便利门面：

```cpp
#include <diagnostics/ModuleLogger.h>
```

`ModuleLogger` 持有一个非拥有型 `ILogger&`，拥有固定模块名，并提供 `trace()`、`debug()`、`info()`、`warning()`、`error()` 和 `critical()`。这些方法支持 fmt 的 `{}` 占位符。前两个头文件对应的 target 都是 `englab::diagnostics`，业务组件不应直接创建 `SpdlogLogger`。

只有组合根或负责组装基础设施的外层代码包含：

```cpp
#include <logging/SpdlogLogger.h>
```

该头文件提供 `SpdlogLoggerOptions` 和 `SpdlogLogger`，对应 target 是 `englab::logging`。它负责选择日志文件、滚动策略和同步/异步模式。

### 4.2 在组合根创建日志对象

进程启动时创建一个 logger，并让所有模块共用 `logs/engineeringlab.log`：

```cpp
#include "AppComposition.h"

#include <logging/SpdlogLogger.h>

using engineeringlab::application::diagnostics::LogLevel;
using engineeringlab::infrastructure::logging::SpdlogLogger;
using engineeringlab::infrastructure::logging::SpdlogLoggerOptions;

SpdlogLoggerOptions options;
options.logFile = "logs/engineeringlab.log";
options.minimumLevel = LogLevel::Info;
options.maxFileSizeBytes = 10U * 1024U * 1024U;
options.maxFiles = 5U;
options.enableConsole = false;
options.asynchronous = true;

SpdlogLogger logger(options);
engineeringlab::composition::AppComposition composition(logger);
```

当前综合工作台使用相对于进程工作目录的 `logs/engineeringlab.log`。`logger` 必须比所有消费者活得更久。不要为 `camera`、`trajectory`、`ui` 等模块分别创建指向同一路径的 `SpdlogLogger`；模块来源由每条记录的 `component` 表达。

### 4.3 业务组件使用 ModuleLogger

```cpp
#include <diagnostics/ModuleLogger.h>

class CameraService {
public:
    explicit CameraService(
        engineeringlab::application::diagnostics::ILogger& logger
    )
        : m_log(logger, "camera")
    {
    }

    void open(int cameraIndex)
    {
        m_log.info("Opening camera {}.", cameraIndex);
    }

private:
    engineeringlab::application::diagnostics::ModuleLogger m_log;
};
```

建议使用稳定、数量有限的 `component` 名称，例如 `ui`、`camera`、`trajectory`、`render` 和 `thread-pool`。它们都会进入同一个日志文件，便于按时间还原跨模块调用过程。

`ModuleLogger` 自己保存模块名，因此每条日志只需要提供格式串和参数：

```cpp
m_log.info("这是日志 {}", std::string("括号里的内容"));
m_log.warning("相机采集超时，等待时间={}ms", timeout);
m_log.error("打开相机失败，错误码={}", errorCode);
```

格式串使用字面量时，fmt 会在编译期检查占位符与参数类型。格式化或后端写入失败不会向业务代码抛出。

### 4.4 需要源码位置时直接使用 ILogger

C++17 没有标准 `std::source_location`，普通成员函数无法自动获得调用者的文件和行号。因此 `ModuleLogger` 的便利方法默认不填写 `SourceLocation`。确实需要源码位置时可直接提交底层记录：

```cpp
logger.write({
    LogLevel::Error,
    "camera",
    "Failed to open camera.",
    {__FILE__, __LINE__, __func__}
});
```

`LogRecord` 中的 `component` 和 `message` 是非拥有型 `std::string_view`。调用方只需保证数据在 `write()` 返回前有效；当前异步实现会在返回前把记录复制进队列。

### 4.5 CMake 依赖

只使用 `ILogger` 的业务 target 链接：

```cmake
target_link_libraries(my_component
    PUBLIC
        englab::diagnostics
)
```

当 `ModuleLogger` 或 `ILogger` 只出现在 `.cpp` 中时可以改用 `PRIVATE`；出现在公共头文件的成员或函数签名中时使用 `PUBLIC`。

创建 `SpdlogLogger` 的组合根链接：

```cmake
target_link_libraries(my_composition_root
    PRIVATE
        englab::logging
)
```

调用方不需要直接链接 `spdlog::spdlog`；该第三方依赖由 `englab::logging` 的实现封装。

### 4.6 刷新和失败处理

正常退出时 `SpdlogLogger` 析构会刷新并排空异步队列。只有在崩溃前、关键设备状态切换等确实需要立即落盘的边界才显式调用 `flush()`，不要在每条日志后刷新。

构造 `SpdlogLogger` 可能因空路径、非法容量或文件创建失败抛出异常。当前 `EngineeringWorkbench` 未配置无日志降级路径，异常会终止启动；若以后需要降级，应在组合根显式提供备用 `ILogger`。构造成功后，`write()` 和 `flush()` 不向业务代码传播后端异常。

## 5. 当前验证

日志相关测试共包含 7 个用例：

- `ModuleLogger` 绑定模块名并完成 `{}` 参数格式化。
- `ModuleLogger` 的六个级别便利方法。
- 拒绝空模块名。
- UTF-8 消息写入与最低级别过滤。
- 异步 logger 析构时排空队列。
- 拒绝空日志文件路径。
- `ModuleLogger` 经 spdlog 后端写入文件，并且无源码位置时不输出空标记。

2026-09-04 本次组合根接入后已完成 Debug 重新配置与构建，CTest 实际发现并通过 27/27 个用例，其中日志相关测试 7/7。启动冒烟确认组合根能够创建 `logs/engineeringlab.log`；由于当前没有业务日志调用，文件为 0 字节。隐藏窗口无法通过 `CloseMainWindow()` 正常退出，进程最终被终止，因此正常交互关闭流程仍未验证。Release 曾在接入前通过构建与 27/27 个用例，本次变更后尚未重新验证；AddressSanitizer 尚未验证。

## 6. 当前接入边界

当前只接通 `qt_main` → `AppComposition` → `CameraComposition` → `CameraCaptureService` 这一条依赖链。组合根拥有唯一的 `SpdlogLogger`，相机服务只保存 `ModuleLogger("camera")`，没有新增业务日志调用；日志对象晚于窗口及其消费者析构。

后续为其他模块接入时继续传递同一个 `ILogger&`，由各模块创建不同 component 的 `ModuleLogger`，不要再创建指向同一文件的后端。

命令行 Usage、课程清单、导出结果路径等标准输出属于程序协议，不应机械替换为日志。相机逐帧回调和 OpenGL 绘制循环也不应写常规 Info 日志。
