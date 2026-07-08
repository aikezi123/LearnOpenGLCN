#pragma once

#include <QWidget>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
class CameraImageCaptureView;
}
QT_END_NAMESPACE

namespace learnopengl::application {
class CameraPreviewService;
}

namespace learnopengl::ui {

class CameraImageCaptureView final : public QWidget {
    Q_OBJECT

public:
    explicit CameraImageCaptureView(
        std::unique_ptr<application::CameraPreviewService> cameraPreview,
        QWidget* parent = nullptr
    );
    ~CameraImageCaptureView() override;

private:
    void connectViewControls();
    float currentZoomScale() const;
    void startCamera();

    Ui::CameraImageCaptureView* m_ui;
    std::unique_ptr<application::CameraPreviewService> m_cameraPreview;
};

} // namespace learnopengl::ui
