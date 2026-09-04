#pragma once

#include <imageframe/ImageFrame.h>

#include <QWidget>

#include <memory>
#include <mutex>

#include <camera/CameraCaptureService.h>

namespace Ui {
class CameraImageCaptureView;
}

class QString;

namespace engineeringlab::ui {

class CameraImageCaptureView final : public QWidget {
    Q_OBJECT

public:
    explicit CameraImageCaptureView(std::unique_ptr<application::CameraCaptureService> cameraCaptureService,QWidget* parent = nullptr);
    ~CameraImageCaptureView() override;

private:
    void connectViewControls();
    float currentZoomScale() const;
    void startCamera();
    void submitLatestFrame(domain::ImageFrame frame);
    void displayLatestFrame();
    void applyCameraParameters();
    void showCameraResult(
        const application::CameraResult& result,
        const QString& successMessage
    );
    void updateCameraControls();

    Ui::CameraImageCaptureView* m_ui;
    std::unique_ptr<application::CameraCaptureService> m_cameraCaptureService;

    // SDK采集线程只覆盖这个槽位，因此UI处理不过来时旧帧会被新帧替换。
    std::mutex m_latestFrameMutex;
    domain::ImageFrame m_latestFrame;

    // true表示已经向Qt事件队列投递了一个显示任务，避免每帧都排队。
    bool m_frameDisplayPending{false};
};

} // namespace engineeringlab::ui
