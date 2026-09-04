#include <logging/SpdlogLogger.h>

#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <stdexcept>
#include <utility>
#include <vector>

namespace engineeringlab::infrastructure::logging {
namespace {

spdlog::level::level_enum toSpdlogLevel(application::diagnostics::LogLevel level) noexcept
{
    using application::diagnostics::LogLevel;

    switch (level) {
    case LogLevel::Trace:
        return spdlog::level::trace;
    case LogLevel::Debug:
        return spdlog::level::debug;
    case LogLevel::Info:
        return spdlog::level::info;
    case LogLevel::Warning:
        return spdlog::level::warn;
    case LogLevel::Error:
        return spdlog::level::err;
    case LogLevel::Critical:
        return spdlog::level::critical;
    }

    return spdlog::level::info;
}

spdlog::filename_t toSpdlogFilename(const std::filesystem::path& path)
{
    // Windows 使用宽字符原生路径，避免中文日志目录在窄字符代码页下丢失信息。
#if defined(_WIN32) && defined(SPDLOG_WCHAR_FILENAMES)
    return path.native();
#else
    return path.string();
#endif
}

std::string_view sourceFileName(const char* file) noexcept
{
    if (file == nullptr || *file == '\0') {
        return {};
    }

    const std::string_view path(file);
    const std::size_t separator = path.find_last_of("/\\");
    return separator == std::string_view::npos ? path : path.substr(separator + 1U);
}

void validateOptions(const SpdlogLoggerOptions& options)
{
    if (options.loggerName.empty()) {
        throw std::invalid_argument("Logger name must not be empty.");
    }
    if (options.logFile.empty()) {
        throw std::invalid_argument("Log file path must not be empty.");
    }
    if (options.maxFileSizeBytes == 0U) {
        throw std::invalid_argument("Maximum log file size must be greater than zero.");
    }
    if (options.maxFiles == 0U) {
        throw std::invalid_argument("Maximum log file count must be greater than zero.");
    }
    if (options.asynchronous && options.asyncQueueCapacity == 0U) {
        throw std::invalid_argument("Asynchronous queue capacity must be greater than zero.");
    }
}

} // namespace

class SpdlogLogger::Impl final {
public:
    explicit Impl(const SpdlogLoggerOptions& options)
    {
        validateOptions(options);

        // 所有 component 共用该滚动文件；component 会在 write() 中写入消息前缀。
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            toSpdlogFilename(options.logFile),
            options.maxFileSizeBytes,
            options.maxFiles
        ));

        if (options.enableConsole) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        }

        if (options.asynchronous) {
            // 使用实例私有线程池，避免依赖或修改 spdlog 的进程级全局线程池。
            m_threadPool = std::make_shared<spdlog::details::thread_pool>(
                options.asyncQueueCapacity,
                1U
            );
            m_logger = std::make_shared<spdlog::async_logger>(
                options.loggerName,
                sinks.begin(),
                sinks.end(),
                m_threadPool,
                // 队列满时阻塞生产者，优先保证诊断记录完整性。
                spdlog::async_overflow_policy::block
            );
        }
        else {
            m_logger = std::make_shared<spdlog::logger>(
                options.loggerName,
                sinks.begin(),
                sinks.end()
            );
        }

        m_logger->set_level(toSpdlogLevel(options.minimumLevel));
        // Warning 及以上级别立即刷新；其他级别由正常缓冲和析构流程写出。
        m_logger->flush_on(spdlog::level::warn);
        m_logger->set_pattern(
            "%Y-%m-%d %H:%M:%S.%e [%l] [tid %t] [%n] %v"
        );
    }

    ~Impl()
    {
        try {
            m_logger->flush();
        }
        catch (...) {
        }

        // 必须先销毁 logger，再销毁其私有线程池；线程池析构会排空队列并回收工作线程。
        m_logger.reset();
        m_threadPool.reset();
    }

    void write(const application::diagnostics::LogRecord& record) noexcept
    {
        try {
            const char* file = record.source.file == nullptr ? "" : record.source.file;
            const char* function =
                record.source.function == nullptr ? "" : record.source.function;
            const std::string_view shortFile = sourceFileName(file);

            if (!shortFile.empty() && record.source.line > 0) {
                m_logger->log(
                    spdlog::source_loc(file, record.source.line, function),
                    toSpdlogLevel(record.level),
                    "[{}] {} ({}:{})",
                    record.component,
                    record.message,
                    shortFile,
                    record.source.line
                );
            }
            else {
                m_logger->log(
                    toSpdlogLevel(record.level),
                    "[{}] {}",
                    record.component,
                    record.message
                );
            }
        }
        catch (...) {
            // 日志失败不能改变业务控制流，遵守 ILogger 的 noexcept 边界。
        }
    }

    void flush() noexcept
    {
        try {
            m_logger->flush();
        }
        catch (...) {
            // flush 失败同样只影响诊断输出，不传播到调用方。
        }
    }

private:
    std::shared_ptr<spdlog::details::thread_pool> m_threadPool;
    std::shared_ptr<spdlog::logger> m_logger;
};

SpdlogLogger::SpdlogLogger(const SpdlogLoggerOptions& options)
    : m_impl(std::make_unique<Impl>(options))
{
}

SpdlogLogger::~SpdlogLogger() = default;

void SpdlogLogger::write(const application::diagnostics::LogRecord& record) noexcept
{
    m_impl->write(record);
}

void SpdlogLogger::flush() noexcept
{
    m_impl->flush();
}

} // namespace engineeringlab::infrastructure::logging
