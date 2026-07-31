#pragma once

#include <QWidget>
#include <memory>
#include <camera/CameraCaptureService.h>

namespace Ui {
class CameraImageCaptureView;
}


namespace learnopengl::ui {

class CameraImageCaptureView final : public QWidget {
    Q_OBJECT

public:
    explicit CameraImageCaptureView(std::unique_ptr<application::CameraCaptureService> cameraCaptureService,QWidget* parent = nullptr);
    ~CameraImageCaptureView() override;

private:
    void connectViewControls();
    float currentZoomScale() const;
    void startCamera();

    Ui::CameraImageCaptureView* m_ui;
    std::unique_ptr<application::CameraCaptureService> m_cameraCaptureService;
};

} // namespace learnopengl::ui
