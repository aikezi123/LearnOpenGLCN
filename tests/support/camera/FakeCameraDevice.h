#pragma once

#include <camera/ICameraDevice.h>

#include <utility>

namespace learnopengl::tests::support::camera {

// CameraCaptureService 后续测试使用的最小内存实现，不访问真实相机 SDK。
class FakeCameraDevice final : public application::ICameraDevice {
public:
    application::CameraResult openFirstCamera() override
    {
        return succeed();
    }

    application::CameraResult openById(const std::string&) override
    {
        return succeed();
    }

    application::CameraResult openCameraByName(std::string) override
    {
        return succeed();
    }

    application::CameraResult startCapture() override
    {
        return succeed();
    }

    application::CameraResult stopCapture() override
    {
        return succeed();
    }

    application::CameraResult close() override
    {
        return succeed();
    }

    application::CameraResult setAutoWhiteBalance(bool) override
    {
        return succeed();
    }

    application::CameraResult setExposeTimeUs(double) override
    {
        return succeed();
    }

    application::CameraResult setGainDb(double) override
    {
        return succeed();
    }

    application::CameraResult setFps(double) override
    {
        return succeed();
    }

    void setFrameCallback(FrameCallback callback) override
    {
        m_frameCallback = std::move(callback);
    }

private:
    static application::CameraResult succeed()
    {
        application::CameraResult result;
        result.succeeded = true;
        return result;
    }

    FrameCallback m_frameCallback;
};

} // namespace learnopengl::tests::support::camera
