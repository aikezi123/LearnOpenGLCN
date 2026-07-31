#pragma once
#include <camera/ICameraDevice.h>
#include <memory>
#include <string>

namespace learnopengl::application {



class CameraCaptureService final{
public:
    // 明确表示IcameraDevice设备的独占权为CameraCaptureService，调用方无法继续持有ICameraDevice对象。
    explicit CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice);
    // m_cameraDevice是智能指针RALL自动析构，因此这里默认析构即可。
    ~CameraCaptureService() = default;

    // unique_ptr不可复制，因此Service也禁止复制
    CameraCaptureService(const CameraCaptureService &) = delete;
    CameraCaptureService& operator=(const CameraCaptureService &) = delete;

    // 当前阶段还没有线程，因此移动Service是安全的
    CameraCaptureService(CameraCaptureService &&) noexcept;
    CameraCaptureService& operator=(CameraCaptureService &&) noexcept;

    CameraResult openFirstCamera();
    CameraResult openCameraById(std::string deviceId);
    CameraResult openCameraByName(std::string deviceName);

    CameraResult startCapture();
    CameraResult stopCapture();
    CameraResult close();

    CameraResult setAutoWhiteBalance(bool enable);
    CameraResult setExposeTimeUs(double usTime);
    CameraResult setGainDb(double gain);
    CameraResult setFps(double fps);

    void setFrameCallback(ICameraDevice::FrameCallback callback);


private:
    std::unique_ptr<ICameraDevice> m_cameraDevice;

};



} // learnopengl::application
