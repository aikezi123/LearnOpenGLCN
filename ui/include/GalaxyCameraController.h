#pragma once

#include <GalaxyIncludes.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace learnopengl::ui {

// 传给 OpenGL 显示层的一帧相机图像。
// 当前阶段统一转换为 RGB24，后续可直接按 GL_RGB / GL_UNSIGNED_BYTE 上传纹理。
struct GalaxyCameraFrame final {
    int width{0};
    int height{0};
    std::uint64_t frameId{0};
    // 独立持有像素数据，避免大恒 SDK 回调返回后复用底层缓冲区导致悬空引用。
    std::vector<unsigned char> rgb24;
};

class GalaxyCameraController;

// 大恒 SDK 的采集回调适配器。
// SDK 收到一帧后调用 DoOnImageCaptured，本类只把帧转交给 GalaxyCameraController 处理。
class GalaxyCaptureHandler final : public ICaptureEventHandler {
public:
    explicit GalaxyCaptureHandler(GalaxyCameraController* controller);
    void DoOnImageCaptured(CImageDataPointer& imageData, void* userParam) override;

private:
    // 不拥有 controller；controller 负责创建、注册、注销并销毁本回调对象。
    GalaxyCameraController* m_controller{nullptr};
};

class GalaxyCameraController final {
public:
    using FrameCallback = std::function<void(GalaxyCameraFrame)>;

    GalaxyCameraController();
    ~GalaxyCameraController();

    GalaxyCameraController(const GalaxyCameraController&) = delete;
    GalaxyCameraController& operator=(const GalaxyCameraController&) = delete;

    // 打开枚举到的第一台大恒相机，方便当前原型阶段快速跑通。
    bool openFirstCamera();
    // 按大恒相机 UserID 打开指定设备，适合后续固定使用某一台相机。
    bool openByUserId(const std::string& userId);
    // 打开数据流、注册图像回调，并向相机下发 AcquisitionStart。
    bool startGrabbing();
    // 停止采集并关闭数据流，但保留已经打开的设备。
    void stopGrabbing();
    // 停采集、关流、关设备，并释放当前对象持有的 Galaxy SDK 运行时。
    void close();

    //设置相机自动白平衡
    void setAutoWhiteBalance(bool flag);   
    //设置相机增益             
    void setGain(double value);       
    //设置相机曝光时间                  
    void setExposureTime(double value);                 

    bool isOpen() const;
    bool isGrabbing() const;
    std::string lastError() const;

    // 回调发生在大恒 SDK 的采集线程中；上层不要在这里直接调用 OpenGL。
    // Qt/OpenGL 显示层应把帧转发到拥有 QOpenGLWidget context 的线程后再上传纹理。
    void setFrameCallback(FrameCallback callback);

private:
    friend class GalaxyCaptureHandler;

    // IGXFactory::Init / Uninit 是 SDK 级别生命周期；当前原型只由本控制器持有。
    bool initializeSdk();
    void uninitializeSdk();
    void cleanupStream();
    // 由 GalaxyCaptureHandler 转发调用，把 SDK 图像转换并复制为 GalaxyCameraFrame。
    void handleFrame(CImageDataPointer& imageData);   // 这个函数是在大恒SDK的线程里运行的，因此此函数里不能直接操作UI和OPenGL的上传
    void setLastError(std::string message);

    // 大恒 C++ SDK 智能指针，分别表示设备、采集流和远端相机 Feature 控制器。
    CGXDevicePointer m_device;
    CGXStreamPointer m_stream;
    CGXFeatureControlPointer m_featureControl;
    std::unique_ptr<GalaxyCaptureHandler> m_captureHandler;

    FrameCallback m_frameCallback;        // 用来通知UI层有原始帧可以取用
    mutable std::mutex m_callbackMutex;   // 用来保护m_frameCallback成员变量，防止大恒相机的采集线程和UI线程同时访问一个std::function
    mutable std::mutex m_errorMutex;
    std::string m_lastError;

    bool m_sdkInitialized{false};
    std::atomic_bool m_isOpen{false};
    std::atomic_bool m_isGrabbing{false};
};

} // namespace learnopengl::ui
