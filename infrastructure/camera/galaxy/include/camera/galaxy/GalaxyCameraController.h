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

    bool openFirstCamera() override;
    bool openByUserId(const std::string& userId) override;
    bool startGrabbing() override;
    void stopGrabbing() override;
    void close() override;

    void setAutoWhiteBalance(bool enabled) override;
    void setGain(double value) override;
    void setExposureTime(double value) override;

    bool isOpen() const override;
    bool isGrabbing() const override;
    std::string lastError() const override;

    void setFrameCallback(FrameCallback callback) override;

private:
    std::unique_ptr<GalaxyCameraControllerImpl> m_impl;
};

} // namespace learnopengl::infrastructure::camera::galaxy
