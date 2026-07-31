#pragma once

#include <camera/ICameraDevice.h>

#include <memory>
#include <string>

namespace learnopengl::infrastructure::camera::galaxy {

class GalaxyCameraControllerImpl;

class GalaxyCameraController final : public application::ICameraDevice {
public:
    GalaxyCameraController();
    ~GalaxyCameraController() override;

    GalaxyCameraController(const GalaxyCameraController&) = delete;
    GalaxyCameraController& operator=(const GalaxyCameraController&) = delete;

    application::CameraResult openFirstCamera() override;
    application::CameraResult openById(const std::string& deviceId) override;
    application::CameraResult openCameraByName(std::string deviceName) override;
    application::CameraResult startCapture() override;
    application::CameraResult stopCapture() override;
    application::CameraResult close() override;

    application::CameraResult setAutoWhiteBalance(bool enable) override;
    application::CameraResult setExposeTimeUs(double usTime) override;
    application::CameraResult setGainDb(double gain) override;
    application::CameraResult setFps(double fps) override;

    void setFrameCallback(FrameCallback callback) override;

private:
    std::unique_ptr<GalaxyCameraControllerImpl> m_impl;
};

} // namespace learnopengl::infrastructure::camera::galaxy
