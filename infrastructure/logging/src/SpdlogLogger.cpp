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
#if defined(_WIN32) && defined(SPDLOG_WCHAR_FILENAMES)
    return path.native();
#else
    return path.string();
#endif
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
            m_threadPool = std::make_shared<spdlog::details::thread_pool>(
                options.asyncQueueCapacity,
                1U
            );
            m_logger = std::make_shared<spdlog::async_logger>(
                options.loggerName,
                sinks.begin(),
                sinks.end(),
                m_threadPool,
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
        m_logger->flush_on(spdlog::level::warn);
        m_logger->set_pattern(
            "%Y-%m-%d %H:%M:%S.%e [%l] [tid %t] [%n] %v (%s:%#)"
        );
    }

    ~Impl()
    {
        try {
            m_logger->flush();
        }
        catch (...) {
        }

        // Destroy the logger before its private async thread pool. The pool
        // drains queued messages and joins its worker during destruction.
        m_logger.reset();
        m_threadPool.reset();
    }

    void write(const application::diagnostics::LogRecord& record) noexcept
    {
        try {
            const char* file = record.source.file == nullptr ? "" : record.source.file;
            const char* function =
                record.source.function == nullptr ? "" : record.source.function;

            m_logger->log(
                spdlog::source_loc(file, record.source.line, function),
                toSpdlogLevel(record.level),
                "[{}] {}",
                record.component,
                record.message
            );
        }
        catch (...) {
        }
    }

    void flush() noexcept
    {
        try {
            m_logger->flush();
        }
        catch (...) {
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
