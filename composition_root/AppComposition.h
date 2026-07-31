#pragma once

#include <memory>

class QMainWindow;

namespace learnopengl::composition {

// 负责创建顶层窗口，并把各功能模块装配到窗口中。
class AppComposition final {
public:
    std::unique_ptr<QMainWindow> createMainWindow() const;
};

} // namespace learnopengl::composition
