#pragma once

#include <imageframe/ImageFrame.h>

#include <functional>
#include <string>

namespace learnopengl::application {

// 描述一次相机设备操作的执行结果。
struct CameraResult final {
    bool succeeded{false};
    std::string errorMessage;
};

// Application 层访问相机设备所需的同步端口。
// 所有控制操作均由 Application 的相机控制线程串行调用，
// 具体 SDK 操作由 Infrastructure 层的相机适配器实现。
class ICameraDevice {
public:
    // 回调可能由相机 SDK 的采集线程触发；ImageFrame 按值传递，
    // 具体实现应优先通过移动转移像素内存所有权。
    using FrameCallback = std::function<void(domain::ImageFrame)>;

    virtual ~ICameraDevice() = default;


    // 打开第一个可用相机。
    virtual CameraResult openFirstCamera() = 0;

    // 根据跨厂商可识别的设备 ID 打开相机。
    virtual CameraResult openById(const std::string& deviceId) = 0;

    // 打开相机自定义名称
    virtual CameraResult openCameraByName(std::string deviceName) = 0;

    // 开始图像采集。
    virtual CameraResult startCapture() = 0;

    // 停止图像采集，但保持设备打开。
    virtual CameraResult stopCapture() = 0;

    // 停止采集并关闭设备。
    virtual CameraResult close() = 0;

    virtual CameraResult setAutoWhiteBalance(bool enable) = 0;
    virtual CameraResult setExposeTimeUs(double usTime) = 0;
    virtual CameraResult setGainDb(double gain) = 0;
    virtual CameraResult setFps(double fps) = 0;

    // 设置空回调表示停止向 Application 投递图像帧。
    virtual void setFrameCallback(FrameCallback callback) = 0;
};

} // namespace learnopengl::application
