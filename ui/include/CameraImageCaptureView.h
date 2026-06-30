#pragma once

#include "GalaxyCameraController.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class CameraImageCaptureView;
}
QT_END_NAMESPACE

namespace learnopengl::ui {

class CameraImageCaptureView final : public QWidget {
    Q_OBJECT

public:
    explicit CameraImageCaptureView(QWidget* parent = nullptr);
    ~CameraImageCaptureView() override;

private:
    void startCamera();

    Ui::CameraImageCaptureView* m_ui;
    GalaxyCameraController m_cameraController;
};

} // namespace learnopengl::ui
