#pragma once

#include <diagnostics/ILogger.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>

namespace engineeringlab::infrastructure::logging {

struct SpdlogLoggerOptions {
    std::string loggerName{"EngineeringLab"};
    std::filesystem::path logFile;
    application::diagnostics::LogLevel minimumLevel{
        application::diagnostics::LogLevel::Info
    };
    std::size_t maxFileSizeBytes{10U * 1024U * 1024U};
    std::size_t maxFiles{5U};
    std::size_t asyncQueueCapacity{8192U};
    bool enableConsole{false};
    bool asynchronous{true};
};

// Concrete logging adapter. spdlog types are hidden behind Pimpl so callers
// only depend on the project-owned ILogger interface.
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
