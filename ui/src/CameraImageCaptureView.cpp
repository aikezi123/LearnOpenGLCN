#include "CameraImageCaptureView.h"

#include "DisplayOpenGLImage.h"
#include "ui_CameraImageCaptureView.h"

#include <camera/CameraPreviewService.h>

#include <QCheckBox>
#include <QMetaObject>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>

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
    connectViewControls();
    startCamera();
}

CameraImageCaptureView::~CameraImageCaptureView()
{
    if (m_cameraPreview != nullptr) {
        m_cameraPreview->close();
    }

    delete m_ui;
}

void CameraImageCaptureView::connectViewControls()
{
    connect(
        m_ui->flipHorizontalCheckBox,
        &QCheckBox::toggled,
        m_ui->widget,
        &DisplayOpenGLImage::setFlipHorizontal
    );
    connect(
        m_ui->flipVerticalCheckBox,
        &QCheckBox::toggled,
        m_ui->widget,
        &DisplayOpenGLImage::setFlipVertical
    );
    connect(
        m_ui->rotateLeftButton,
        &QPushButton::clicked,
        m_ui->widget,
        &DisplayOpenGLImage::rotateCounterClockwise90
    );
    connect(
        m_ui->rotateRightButton,
        &QPushButton::clicked,
        m_ui->widget,
        &DisplayOpenGLImage::rotateClockwise90
    );
    connect(m_ui->zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        m_ui->zoomValueLabel->setText(QStringLiteral("%1%").arg(value));
        m_ui->widget->setViewScale(currentZoomScale());
    });

    constexpr float panStep = 0.08F;
    connect(m_ui->panLeftButton, &QPushButton::clicked, this, [this]() {
        m_ui->widget->panView(-panStep, 0.0F);
    });
    connect(m_ui->panRightButton, &QPushButton::clicked, this, [this]() {
        m_ui->widget->panView(panStep, 0.0F);
    });
    connect(m_ui->panUpButton, &QPushButton::clicked, this, [this]() {
        m_ui->widget->panView(0.0F, panStep);
    });
    connect(m_ui->panDownButton, &QPushButton::clicked, this, [this]() {
        m_ui->widget->panView(0.0F, -panStep);
    });
    connect(m_ui->resetViewButton, &QPushButton::clicked, this, [this]() {
        const QSignalBlocker blockFlipHorizontal(m_ui->flipHorizontalCheckBox);
        const QSignalBlocker blockFlipVertical(m_ui->flipVerticalCheckBox);
        const QSignalBlocker blockZoom(m_ui->zoomSlider);

        m_ui->flipHorizontalCheckBox->setChecked(false);
        m_ui->flipVerticalCheckBox->setChecked(false);
        m_ui->zoomSlider->setValue(100);
        m_ui->zoomValueLabel->setText(QStringLiteral("100%"));
        m_ui->widget->resetViewTransform();
    });
}

float CameraImageCaptureView::currentZoomScale() const
{
    return static_cast<float>(m_ui->zoomSlider->value()) / 100.0F;
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
