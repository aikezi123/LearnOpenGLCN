#pragma once

class QWidget;

namespace learnopengl::composition {

// 创建轨迹功能对应的 UI 页面。
class TrajectoryComposition final {
public:
    static QWidget* createPage(QWidget* parent = nullptr);
};

} // namespace learnopengl::composition
