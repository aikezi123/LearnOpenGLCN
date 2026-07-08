#pragma once

#include <camera/ICameraDevice.h>

#include <memory>
#include <string>

namespace learnopengl::application {

class CameraPreviewService final {
public:
    explicit CameraPreviewService(std::unique_ptr<ICameraDevice> cameraDevice);
    ~CameraPreviewService();

    CameraPreviewService(const CameraPreviewService&) = delete;
    CameraPreviewService& operator=(const CameraPreviewService&) = delete;

    bool startPreview();
    bool startPreviewByUserId(const std::string& userId);
    void stopPreview();
    void close();

    void setAutoWhiteBalance(bool enabled);
    void setGain(double value);
    void setExposureTime(double value);

    bool isOpen() const;
    bool isGrabbing() const;
    std::string lastError() const;

    void setFrameCallback(ICameraDevice::FrameCallback callback);

private:
    std::unique_ptr<ICameraDevice> m_cameraDevice;
};

} // namespace learnopengl::application
