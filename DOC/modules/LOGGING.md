# 日志模块

## 1. 当前状态

工程已加入日志接口和 spdlog 基础设施实现，但生产代码尚未创建、注入或调用日志对象。`EngineeringWorkbench`、`OpenGLLessons`、UI、相机、OpenGL 课程和命令行工具的运行行为保持不变。

当前 target：

| Target | 类型 | 职责 |
| --- | --- | --- |
| `englab::diagnostics` | Interface | 定义 `LogLevel`、`LogRecord`、`SourceLocation` 和 `ILogger` |
| `englab::logging` | Static | 用 spdlog 实现异步/同步、滚动文件和可选控制台日志 |

依赖方向为：

```text
englab::logging -> englab::diagnostics
englab::logging -> spdlog::spdlog provided by vcpkg (PRIVATE)
```

spdlog 类型通过 `SpdlogLogger` 的 Pimpl 隐藏，不进入 application 公共日志端口。domain 不依赖日志模块。

## 2. 第三方依赖

工程通过根目录 `vcpkg.json` 使用 spdlog 1.17.0，版本由固定的 vcpkg `builtin-baseline` 决定。仓库不再保存 spdlog 的源码、头文件或预编译库；CMake 通过 `find_package(spdlog CONFIG REQUIRED)` 获取标准 target `spdlog::spdlog`。

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

## 4. 当前验证

`engineeringlab_infrastructure_logging_tests` 包含 3 个用例：

- UTF-8 消息写入与最低级别过滤。
- 异步 logger 析构时排空队列。
- 拒绝空日志文件路径。

2026-09-04 已完成 Debug 和 Release 全量构建；两个配置的 CTest 均实际发现并通过 23/23 个用例，其中日志模块 3/3。AddressSanitizer 尚未验证。

## 5. 后续接入边界

当前阶段不修改任何业务调用点。后续启用时由 `composition_root` 选择日志目录、创建一个进程级 `SpdlogLogger`，并按 `ILogger` 注入需要记录诊断信息的外层组件。日志对象必须晚于其消费者析构。

命令行 Usage、课程清单、导出结果路径等标准输出属于程序协议，不应机械替换为日志。相机逐帧回调和 OpenGL 绘制循环也不应写常规 Info 日志。
