#pragma once
#include <camera/ICameraDevice.h>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

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

    // 控制线程保存了this指针，移动对象会使该指针失效，因此禁止移动
    CameraCaptureService(CameraCaptureService &&) = delete;
    CameraCaptureService& operator=(CameraCaptureService &&) = delete;

    // 请求控制线程退出，并等待线程结束
    void shutdown();

private:
    // 控制线程的主函数
    // cameraDevice按值传入，使设备所有权从构造线程转移到控制线程的函数参数中。
    // run()返回时，设备也在控制线程析构。
    void run(std::unique_ptr<ICameraDevice> cameraDevice);

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
    std::thread m_controlThread;

    // 条件变量、锁以及锁保护的变量通常组合使用。
    std::mutex m_mutex;
    std::condition_variable m_conditionVariable;
    bool m_shutdownRequested{false};                // 控制线程是否退出

};



} // learnopengl::application
