#pragma once

#include <QMainWindow>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace learnopengl::application {
class CameraPreviewService;
}

namespace learnopengl::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(
        std::unique_ptr<application::CameraPreviewService> cameraPreview,
        QWidget* parent = nullptr
    );
    ~MainWindow() override;

private:
    Ui::MainWindow* m_ui;
};

} // namespace learnopengl::ui
