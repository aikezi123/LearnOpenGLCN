#include "CameraImageCaptureView.h"

#include "ui_CameraImageCaptureView.h"

#include <camera/CameraPreviewService.h>

#include <QMetaObject>

#include <iostream>
#include <utility>

namespace learnopengl::ui {

CameraImageCaptureView::CameraImageCaptureView(
    std::unique_ptr<application::CameraPreviewService> cameraPreview,
    QWidget* parent
)
    : QWidget(parent)
    , m_ui(new Ui::CameraImageCaptureView)
    , m_cameraPreview(std::move(cameraPreview))
{
    m_ui->setupUi(this);
    startCamera();
}

CameraImageCaptureView::~CameraImageCaptureView()
{
    if (m_cameraPreview != nullptr) {
        m_cameraPreview->close();
    }

    delete m_ui;
}

void CameraImageCaptureView::startCamera()
{
    if (m_cameraPreview == nullptr) {
        std::cerr << "Camera preview service is not configured." << std::endl;
        return;
    }

    m_cameraPreview->setFrameCallback([this](learnopengl::domain::VideoFrame frame) {
        QMetaObject::invokeMethod(
            this,
            [this, frame = std::move(frame)]() mutable {
                m_ui->widget->setRgb24Frame(
                    frame.width,
                    frame.height,
                    std::move(frame.pixels)
                );
            },
            Qt::QueuedConnection
        );
    });

    if (!m_cameraPreview->startPreview()) {
        std::cerr << m_cameraPreview->lastError() << std::endl;
    }
}

} // namespace learnopengl::ui
