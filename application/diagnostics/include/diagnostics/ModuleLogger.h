#pragma once

#include <diagnostics/ILogger.h>

#include <fmt/format.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace engineeringlab::application::diagnostics {

// 绑定一个逻辑模块名的轻量日志门面。
//
// ModuleLogger 不拥有 ILogger，调用方必须保证底层 logger 的生命周期更长。模块名由本
// 对象持有，因此每次记录日志时不需要重复传入。格式化或后端写入失败不会逃逸到业务
// 代码。该 C++17 便利接口不自动捕获调用位置；需要源码位置时可直接使用 ILogger。
class ModuleLogger final {
public:
    ModuleLogger(ILogger& logger, std::string component)
        : m_logger(logger)
        , m_component(std::move(component))
    {
        if (m_component.empty()) {
            throw std::invalid_argument("Log component must not be empty.");
        }
    }

    std::string_view component() const noexcept
    {
        return m_component;
    }

    void trace(std::string_view message) const noexcept
    {
        write(LogLevel::Trace, message);
    }

    template <typename... Args>
    void trace(fmt::format_string<Args...> format, Args&&... args) const noexcept
    {
        writeFormatted(LogLevel::Trace, format, std::forward<Args>(args)...);
    }

    void debug(std::string_view message) const noexcept
    {
        write(LogLevel::Debug, message);
    }

    template <typename... Args>
    void debug(fmt::format_string<Args...> format, Args&&... args) const noexcept
    {
        writeFormatted(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

    void info(std::string_view message) const noexcept
    {
        write(LogLevel::Info, message);
    }

    template <typename... Args>
    void info(fmt::format_string<Args...> format, Args&&... args) const noexcept
    {
        writeFormatted(LogLevel::Info, format, std::forward<Args>(args)...);
    }

    void warning(std::string_view message) const noexcept
    {
        write(LogLevel::Warning, message);
    }

    template <typename... Args>
    void warning(fmt::format_string<Args...> format, Args&&... args) const noexcept
    {
        writeFormatted(LogLevel::Warning, format, std::forward<Args>(args)...);
    }

    void error(std::string_view message) const noexcept
    {
        write(LogLevel::Error, message);
    }

    template <typename... Args>
    void error(fmt::format_string<Args...> format, Args&&... args) const noexcept
    {
        writeFormatted(LogLevel::Error, format, std::forward<Args>(args)...);
    }

    void critical(std::string_view message) const noexcept
    {
        write(LogLevel::Critical, message);
    }

    template <typename... Args>
    void critical(fmt::format_string<Args...> format, Args&&... args) const noexcept
    {
        writeFormatted(LogLevel::Critical, format, std::forward<Args>(args)...);
    }

private:
    void write(LogLevel level, std::string_view message) const noexcept
    {
        m_logger.write(LogRecord{level, m_component, message, {}});
    }

    template <typename... Args>
    void writeFormatted(
        LogLevel level,
        fmt::format_string<Args...> format,
        Args&&... args
    ) const noexcept
    {
        try {
            const std::string message = fmt::format(
                format,
                std::forward<Args>(args)...
            );
            write(level, message);
        }
        catch (...) {
            // 日志格式化失败不能改变业务控制流。
        }
    }

    ILogger& m_logger;
    std::string m_component;
};

} // namespace engineeringlab::application::diagnostics
