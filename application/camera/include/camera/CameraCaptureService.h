#pragma once
#include <camera/ICameraDevice.h>
#include <future>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <functional>
#include <memory>

namespace engineeringlab::application {

class CameraCaptureService final {
public:
    explicit CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice);
    ~CameraCaptureService();

    CameraCaptureService(const CameraCaptureService &) = delete;
    CameraCaptureService& operator=(const CameraCaptureService &) = delete;
    CameraCaptureService(CameraCaptureService &&) = delete;
    CameraCaptureService& operator=(CameraCaptureService &&) = delete;

    void run(std::unique_ptr<ICameraDevice> cameraDevice);
    void shutdown();

public:
    enum class State {
        Closed,
        Captured,
        Opened
    };
    State state() const;

    std::future<CameraResult> requestOpen();
    std::future<CameraResult> requestOpenById(std::string deviceId);
    std::future<CameraResult> requestOpenByName(std::string deviceName);
    std::future<CameraResult> requestClose();
    std::future<CameraResult> requestStartCapture();
    std::future<CameraResult> requestStopCaptrue();
    std::future<CameraResult> requestSetAutoWhiteBalance(bool enable);
    std::future<CameraResult> requestSetExposeTimeUs(double usTime);
    std::future<CameraResult> requestSetGainDb(double gain);
    std::future<CameraResult> requestSetFps(double fps);
    std::future<CameraResult> requestSetFrameCallback(ICameraDevice::FrameCallback callback);

private:
    // 控制命令动作
    using CommandAction = std::function<CameraResult(ICameraDevice &device)>;
    
    // 控制命令，包含命令动作以及异步的控制结果
    struct Command {
        CommandAction action;  
        std::promise<CameraResult> promiseResult; // 异步命令结果，用std::promise写入
    };

    // 将控制命令动作和std::promise一起打包到队列中，控制线程会取出来操作，CameraCaptureService所在线程可取得对应的future对象。
    std::future<CameraResult> submit(CommandAction action);
    
private:
    // 锁保护请求退出标志位以及队列
    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_shutdownRequest{false};
    std::queue<Command> m_commands;

    // 设备状态用原子量，CameraCaptureService也可以知道当前设备状态
    std::atomic<State> m_state{State::Closed};
    std::thread m_thread;


};




} // engineeringlab::application
