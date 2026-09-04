#pragma once

#include <string_view>

namespace engineeringlab::application::diagnostics {

// 日志严重级别。调用方负责选择语义，具体后端负责最低级别过滤。
enum class LogLevel {
    Trace,   // 最细粒度的流程信息，通常只用于短期定位执行路径或高频内部状态。
    Debug,   // 面向开发和调试的状态信息，Release 环境通常会过滤该级别。
    Info,    // 程序正常运行中的关键事件，例如启动完成、设备连接或任务结束。
    Warning, // 出现异常情况但程序仍可继续运行，例如重试、降级或采用默认配置。
    Error,   // 当前操作已经失败，但程序或其他功能通常仍能继续运行。
    Critical // 严重故障，表示进程、核心模块或关键资源已经无法继续可靠工作。
};

// 可选的源码位置。空指针和行号 0 表示调用方没有提供该信息。
// 指针只需在 ILogger::write() 返回前保持有效。
struct SourceLocation {
    const char* file{nullptr};     // 源文件名，通常传入 __FILE__；nullptr 表示不记录源文件。
    int line{0};                   // 源代码行号，通常传入 __LINE__；0 表示未提供有效行号。
    const char* function{nullptr}; // 函数名，通常传入 __func__；nullptr 表示不记录函数名。
};

// 一次日志调用的非拥有型视图。
//
// component 是稳定的逻辑模块名（例如 "camera" 或 "trajectory"），用于在同一日志
// 文件中检索来源，并不表示要为该模块创建单独文件。message 和 component 指向的内容
// 只需在 write() 返回前有效；异步实现必须在返回前复制所需数据。
struct LogRecord {
    LogLevel level{LogLevel::Info};  // 本条记录的严重级别，后端会根据最低日志级别判断是否输出。
    std::string_view component;      // 产生日志的逻辑模块名，例如 "camera"、"trajectory" 或 "render"。
    std::string_view message;        // 面向开发者和维护人员的 UTF-8 日志正文。
    SourceLocation source;           // 可选的代码来源，用于输出文件、行号和函数名。
};

// 面向 application 及其调用方的日志端口，不暴露任何第三方日志类型。
//
// 实现必须允许不同线程并发调用，并且不得让写入或刷新失败逃逸到业务、UI 或设备控制
// 代码。日志对象由组合根持有，其生命周期必须覆盖所有使用它的组件。
class ILogger {
public:
    virtual ~ILogger() = default;

    // 提交一条日志。实现应在返回前消费或复制 LogRecord 中的所有非拥有型数据。
    virtual void write(const LogRecord& record) noexcept = 0;

    // 请求尽快写出已经提交的日志；后端失败不会向调用方抛出。
    virtual void flush() noexcept = 0;
};

} // namespace engineeringlab::application::diagnostics
