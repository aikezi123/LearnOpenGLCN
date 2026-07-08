#pragma once

#include <video/VideoFrame.h>

#include <functional>
#include <string>

namespace learnopengl::application {

class ICameraDevice {
public:
    using FrameCallback = std::function<void(domain::VideoFrame)>;

    virtual ~ICameraDevice() = default;

    virtual bool openFirstCamera() = 0;
    virtual bool openByUserId(const std::string& userId) = 0;
    virtual bool startGrabbing() = 0;
    virtual void stopGrabbing() = 0;
    virtual void close() = 0;

    virtual void setAutoWhiteBalance(bool enabled) = 0;
    virtual void setGain(double value) = 0;
    virtual void setExposureTime(double value) = 0;

    virtual bool isOpen() const = 0;
    virtual bool isGrabbing() const = 0;
    virtual std::string lastError() const = 0;

    virtual void setFrameCallback(FrameCallback callback) = 0;
};

} // namespace learnopengl::application
