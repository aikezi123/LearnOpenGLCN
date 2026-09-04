#pragma once

#include <memory>

class QMainWindow;

namespace engineeringlab::application::diagnostics {
class ILogger;
}

namespace engineeringlab::composition {

// 负责创建顶层窗口，并把各功能模块装配到窗口中。
class AppComposition final {
public:
    explicit AppComposition(application::diagnostics::ILogger& logger) noexcept;

    std::unique_ptr<QMainWindow> createMainWindow() const;

private:
    // 非拥有引用；qt_main 中的进程级日志后端必须晚于本对象及窗口析构。
    application::diagnostics::ILogger& m_logger;
};

} // namespace engineeringlab::composition
