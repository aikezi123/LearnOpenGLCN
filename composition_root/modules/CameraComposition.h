#pragma once

class QWidget;

namespace learnopengl::composition {

// 创建相机基础设施实现、Application Service 和对应的 UI 页面。
class CameraComposition final {
public:
    static QWidget* createPage(QWidget* parent = nullptr);
};

} // namespace learnopengl::composition
