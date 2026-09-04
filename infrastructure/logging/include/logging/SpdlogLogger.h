#pragma once

#include <diagnostics/ILogger.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace engineeringlab::infrastructure::logging {

// spdlog 适配器的进程级配置。
// 推荐由组合根创建一个 SpdlogLogger，让所有模块通过 LogRecord::component 共用同一个
// 滚动文件；不要为每个业务模块重复创建指向同一文件的 logger。
struct SpdlogLoggerOptions {
    // 出现在日志格式 [%n] 中的应用级 logger 名称，不是业务模块名。
    std::string loggerName{"EngineeringLab"};

    // 当前进程的主日志文件路径，例如 logs/engineeringlab.log。
    std::filesystem::path logFile;

    // 低于该级别的记录会被后端过滤。
    application::diagnostics::LogLevel minimumLevel{
        application::diagnostics::LogLevel::Info
    };

    // 滚动文件策略：当前文件达到上限后轮转，并按 maxFiles 限制历史文件数量。
    std::size_t maxFileSizeBytes{10U * 1024U * 1024U};
    std::size_t maxFiles{5U};

    // 异步队列按日志条数计数；队列满时实现会阻塞提交线程，避免静默丢失日志。
    std::size_t asyncQueueCapacity{8192U};

    // 开启后，同一条记录还会输出到彩色控制台。
    bool enableConsole{false};

    // 默认使用私有后台线程写日志；关闭后由调用线程同步写入。
    bool asynchronous{true};
};

// ILogger 的 spdlog 实现。Pimpl 隐藏全部 spdlog 类型，使普通调用方只依赖项目接口。
// 构造函数会校验配置，并可能因文件创建等初始化失败抛出异常；write() 和 flush() 保持
// ILogger 的 noexcept 约定。对象不可复制或移动，以固定异步线程池和 logger 的生命周期。
class SpdlogLogger final : public application::diagnostics::ILogger {
public:
    explicit SpdlogLogger(const SpdlogLoggerOptions& options);
    ~SpdlogLogger() override;

    SpdlogLogger(const SpdlogLogger&) = delete;
    SpdlogLogger& operator=(const SpdlogLogger&) = delete;
    SpdlogLogger(SpdlogLogger&&) = delete;
    SpdlogLogger& operator=(SpdlogLogger&&) = delete;

    void write(const application::diagnostics::LogRecord& record) noexcept override;
    void flush() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace engineeringlab::infrastructure::logging
