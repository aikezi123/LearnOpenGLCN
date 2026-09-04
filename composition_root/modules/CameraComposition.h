#pragma once

class QWidget;

namespace engineeringlab::application::diagnostics {
class ILogger;
}

namespace engineeringlab::composition {

// 创建相机基础设施实现、Application Service 和对应的 UI 页面。
class CameraComposition final {
public:
    static QWidget* createPage(
        application::diagnostics::ILogger& logger,
        QWidget* parent = nullptr
    );
};

} // namespace engineeringlab::composition
