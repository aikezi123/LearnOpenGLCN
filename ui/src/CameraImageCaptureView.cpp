#include "CameraImageCaptureView.h"

#include "ui_CameraImageCaptureView.h"

#include <QMetaObject>

#include <iostream>
#include <utility>

namespace learnopengl::ui {

CameraImageCaptureView::CameraImageCaptureView(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::CameraImageCaptureView)
{
    m_ui->setupUi(this);
    startCamera();
}

CameraImageCaptureView::~CameraImageCaptureView()
{
    m_cameraController.close();
    delete m_ui;
}

void CameraImageCaptureView::startCamera()
{
    m_cameraController.setFrameCallback([this](GalaxyCameraFrame frame) {
        QMetaObject::invokeMethod(
            this,
            [this, frame = std::move(frame)]() mutable {
                m_ui->widget->setRgb24Frame(
                    frame.width,
                    frame.height,
                    std::move(frame.rgb24)
                );
            },
            Qt::QueuedConnection
        );
    });

    if (!m_cameraController.openFirstCamera()) {
        std::cerr << m_cameraController.lastError() << std::endl;
        return;
    }

    if (!m_cameraController.startGrabbing()) {
        std::cerr << m_cameraController.lastError() << std::endl;
    }
}

} // namespace learnopengl::ui
