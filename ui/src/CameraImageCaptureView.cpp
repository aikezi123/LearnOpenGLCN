#include "CameraImageCaptureView.h"
#include "DisplayOpenGLImage.h"
#include "ui_CameraImageCaptureView.h"
#include <camera/CameraCaptureService.h>
#include <imageframe/ImageFrame.h>


#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>
#include <QStringList>

#include <iostream>
#include <utility>

namespace learnopengl::ui {

CameraImageCaptureView::CameraImageCaptureView(std::unique_ptr<application::CameraCaptureService> cameraCaptureService,QWidget* parent)
    : QWidget(parent) , m_ui(new Ui::CameraImageCaptureView) , m_cameraCaptureService(std::move(cameraCaptureService))
{
    m_ui->setupUi(this);
    connectViewControls();
    updateCameraControls();

    // 保留原来的行为：页面创建后自动打开第一台相机并开始采集。
    startCamera();
}

CameraImageCaptureView::~CameraImageCaptureView()
{
    if (m_cameraCaptureService != nullptr) {
        // 先注销帧回调，避免相机线程继续向正在析构的 QWidget 投递图像。
        const application::CameraResult callbackResult =
            m_cameraCaptureService->requestSetFrameCallback({}).get();
        if (!callbackResult.succeeded) {
            std::cerr << callbackResult.errorMessage << std::endl;
        }

        if (m_cameraCaptureService->state()
            != application::CameraCaptureService::State::Closed) {
            const application::CameraResult closeResult =
                m_cameraCaptureService->requestClose().get();
            if (!closeResult.succeeded) {
                std::cerr << closeResult.errorMessage << std::endl;
            }
        }
    }

    delete m_ui;
}

void CameraImageCaptureView::connectViewControls()
{
    connect(m_ui->flipHorizontalCheckBox,&QCheckBox::toggled, m_ui->widget, &DisplayOpenGLImage::setFlipHorizontal);
    connect(m_ui->flipVerticalCheckBox, &QCheckBox::toggled, m_ui->widget, &DisplayOpenGLImage::setFlipVertical);
    connect(m_ui->circleDisplayCheckBox, &QCheckBox::toggled, this, [this](bool enabled) {
        m_ui->widget->setDisplayShape(
            enabled
                ? DisplayOpenGLImage::DisplayShape::Circle
                : DisplayOpenGLImage::DisplayShape::Rectangle
        );
    });
    connect(m_ui->rotateLeftButton, &QPushButton::clicked, m_ui->widget, &DisplayOpenGLImage::rotateCounterClockwise90);
    connect(m_ui->rotateRightButton, &QPushButton::clicked, m_ui->widget, &DisplayOpenGLImage::rotateClockwise90);
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

    connect(m_ui->openFirstCameraButton, &QPushButton::clicked, this, [this]() {
        showCameraResult(
            m_cameraCaptureService->requestOpen().get(),
            QStringLiteral("相机已打开")
        );
    });
    connect(m_ui->openByIdButton, &QPushButton::clicked, this, [this]() {
        showCameraResult(
            m_cameraCaptureService->requestOpenById(
                m_ui->cameraIdEdit->text().trimmed().toStdString()
            ).get(),
            QStringLiteral("已按ID打开相机")
        );
    });
    connect(m_ui->openByNameButton, &QPushButton::clicked, this, [this]() {
        showCameraResult(
            m_cameraCaptureService->requestOpenByName(
                m_ui->cameraNameEdit->text().trimmed().toStdString()
            ).get(),
            QStringLiteral("已按名称打开相机")
        );
    });
    connect(m_ui->startCaptureButton, &QPushButton::clicked, this, [this]() {
        showCameraResult(
            m_cameraCaptureService->requestStartCapture().get(),
            QStringLiteral("正在采集图像")
        );
    });
    connect(m_ui->stopCaptureButton, &QPushButton::clicked, this, [this]() {
        showCameraResult(
            m_cameraCaptureService->requestStopCaptrue().get(),
            QStringLiteral("已停止采集")
        );
    });
    connect(m_ui->closeCameraButton, &QPushButton::clicked, this, [this]() {
        showCameraResult(
            m_cameraCaptureService->requestClose().get(),
            QStringLiteral("相机已关闭")
        );
    });
    connect(
        m_ui->applyCameraParametersButton,
        &QPushButton::clicked,
        this,
        &CameraImageCaptureView::applyCameraParameters
    );
}

float CameraImageCaptureView::currentZoomScale() const
{
    return static_cast<float>(m_ui->zoomSlider->value()) / 100.0F;
}

void CameraImageCaptureView::startCamera()
{
    if (m_cameraCaptureService == nullptr) {
        std::cerr << "Camera capture service is not configured." << std::endl;
        return;
    }

    application::CameraResult result = m_cameraCaptureService->requestSetFrameCallback(
        [this](domain::ImageFrame frame) {
            submitLatestFrame(std::move(frame));
        }
    ).get();

    if (!result.succeeded) {
        std::cerr << result.errorMessage << std::endl;
        return;
    }

    result = m_cameraCaptureService->requestOpen().get();
    if (!result.succeeded) {
        showCameraResult(result, {});
        return;
    }

    result = m_cameraCaptureService->requestStartCapture().get();
    showCameraResult(result, QStringLiteral("正在采集图像"));
}

void CameraImageCaptureView::submitLatestFrame(domain::ImageFrame frame)
{
    if (frame.pixelFormat != domain::PixelFormat::Rgb24) {
        return;
    }

    bool needPostDisplayTask = false;
    {
        std::lock_guard<std::mutex> lock(m_latestFrameMutex);

        // 永远覆盖尚未显示的旧帧，避免相机帧在Qt事件队列中不断累积。
        m_latestFrame = std::move(frame);

        if (!m_frameDisplayPending) {
            m_frameDisplayPending = true;
            needPostDisplayTask = true;
        }
    }

    if (!needPostDisplayTask) {
        return;
    }

    // 相机回调可能来自SDK线程；Qt只排入一个任务，任务执行时再取最新帧。
    const bool posted = QMetaObject::invokeMethod(
        this,
        [this]() {
            displayLatestFrame();
        },
        Qt::QueuedConnection
    );

    if (!posted) {
        // 投递失败时允许下一帧重新尝试，不让pending标志永久保持true。
        std::lock_guard<std::mutex> lock(m_latestFrameMutex);
        m_frameDisplayPending = false;
    }
}

void CameraImageCaptureView::displayLatestFrame()
{
    domain::ImageFrame frame;
    {
        std::lock_guard<std::mutex> lock(m_latestFrameMutex);

        frame = std::move(m_latestFrame);
        m_frameDisplayPending = false;
    }

    if (frame.width <= 0 || frame.height <= 0 || frame.pixels.empty()) {
        return;
    }

    m_ui->widget->setRgb24Frame(
        frame.width,
        frame.height,
        std::move(frame.pixels)
    );
}

void CameraImageCaptureView::applyCameraParameters()
{
    if (m_cameraCaptureService == nullptr) {
        return;
    }

    QStringList errorMessages;

    application::CameraResult result =
        m_cameraCaptureService->requestSetAutoWhiteBalance(
            m_ui->autoWhiteBalanceCheckBox->isChecked()
        ).get();
    if (!result.succeeded) {
        errorMessages.push_back(QString::fromStdString(result.errorMessage));
    }

    result = m_cameraCaptureService->requestSetExposeTimeUs(
        m_ui->exposureSpinBox->value()
    ).get();
    if (!result.succeeded) {
        errorMessages.push_back(QString::fromStdString(result.errorMessage));
    }

    result = m_cameraCaptureService->requestSetGainDb(
        m_ui->gainSpinBox->value()
    ).get();
    if (!result.succeeded) {
        errorMessages.push_back(QString::fromStdString(result.errorMessage));
    }

    result = m_cameraCaptureService->requestSetFps(
        m_ui->fpsSpinBox->value()
    ).get();
    if (!result.succeeded) {
        errorMessages.push_back(QString::fromStdString(result.errorMessage));
    }

    if (errorMessages.isEmpty()) {
        showCameraResult(result, QStringLiteral("相机参数已应用"));
        return;
    }

    application::CameraResult combinedResult;
    combinedResult.errorMessage = errorMessages.join(QStringLiteral("；")).toStdString();
    showCameraResult(combinedResult, {});
}

void CameraImageCaptureView::showCameraResult(
    const application::CameraResult& result,
    const QString& successMessage
)
{
    if (result.succeeded) {
        m_ui->cameraStatusLabel->setText(successMessage);
        m_ui->cameraStatusLabel->setStyleSheet(QStringLiteral("color: #207a37;"));
    }
    else {
        const QString errorMessage = QString::fromStdString(result.errorMessage);
        m_ui->cameraStatusLabel->setText(errorMessage);
        m_ui->cameraStatusLabel->setStyleSheet(QStringLiteral("color: #b42318;"));
        std::cerr << result.errorMessage << std::endl;
    }

    updateCameraControls();
}

void CameraImageCaptureView::updateCameraControls()
{
    if (m_cameraCaptureService == nullptr) {
        m_ui->cameraControlPanel->setEnabled(false);
        return;
    }

    const application::CameraCaptureService::State state = m_cameraCaptureService->state();
    const bool isClosed = state == application::CameraCaptureService::State::Closed;
    const bool isCaptured = state == application::CameraCaptureService::State::Captured;

    m_ui->openFirstCameraButton->setEnabled(isClosed);
    m_ui->openByIdButton->setEnabled(isClosed);
    m_ui->openByNameButton->setEnabled(isClosed);
    m_ui->cameraIdEdit->setEnabled(isClosed);
    m_ui->cameraNameEdit->setEnabled(isClosed);
    m_ui->startCaptureButton->setEnabled(!isClosed && !isCaptured);
    m_ui->stopCaptureButton->setEnabled(isCaptured);
    m_ui->closeCameraButton->setEnabled(!isClosed);
    m_ui->applyCameraParametersButton->setEnabled(!isClosed);
}

} // namespace learnopengl::ui
