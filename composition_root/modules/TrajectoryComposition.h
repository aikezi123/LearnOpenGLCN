#pragma once

class QWidget;

namespace engineeringlab::composition {

// 创建轨迹功能对应的 UI 页面。
class TrajectoryComposition final {
public:
    static QWidget* createPage(QWidget* parent = nullptr);
};

} // namespace engineeringlab::composition
