#pragma once

#include <string_view>

namespace engineeringlab::application::diagnostics {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

struct SourceLocation {
    const char* file{nullptr};
    int line{0};
    const char* function{nullptr};
};

struct LogRecord {
    LogLevel level{LogLevel::Info};
    std::string_view component;
    std::string_view message;
    SourceLocation source;
};

// Application-facing logging port. Implementations must not let logging
// failures escape into business, UI, or device-control code.
class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void write(const LogRecord& record) noexcept = 0;
    virtual void flush() noexcept = 0;
};

} // namespace engineeringlab::application::diagnostics
