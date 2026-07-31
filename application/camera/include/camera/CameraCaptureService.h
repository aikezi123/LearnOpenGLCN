#pragma once
#include <camera/ICameraDevice.h>
#include <memory>
#include <string>

namespace learnopengl::application {

class CameraCaptureService final{
public:
    explicit CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice);
    ~CameraCaptureService() = default;

    // unique_ptr不可复制，因此Service也禁止复制
    CameraCaptureService(const CameraCaptureService &) = delete;
    CameraCaptureService& operator=(const CameraCaptureService &) = delete;

    // 当前阶段还没有线程，因此移动Service是安全的
    CameraCaptureService(CameraCaptureService &&) noexcept = delete;
    CameraCaptureService& operator=(CameraCaptureService &&) noexcept = delete;

private:
    std::unique_ptr<ICameraDevice> m_cameraDevice;

};



} // learnopengl::application