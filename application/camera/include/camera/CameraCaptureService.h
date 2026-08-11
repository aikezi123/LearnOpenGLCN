#pragma once
#include <camera/ICameraDevice.h>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace learnopengl::application {

class CameraCaptureService final {
public:
    explicit CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice);
    ~CameraCaptureService();


    CameraCaptureService(const CameraCaptureService &) = delete;
    CameraCaptureService& operator=(const CameraCaptureService&) = delete;
    CameraCaptureService(CameraCaptureService &&) = delete;
    CameraCaptureService& operator=(CameraCaptureService &&) = delete;

public:
    enum class State {
        Closed,
        Opened,
        Capturing
    };
    State state() const noexcept;
    bool requestOpen();
    bool requestClose();
    bool requestStartCapture();
    bool requestStopCapture();
    bool requestSetFrameCallback(ICameraDevice::FrameCallback callback);

    void shutdown();

private:
    using Command = std::function<void(ICameraDevice &cameraDevice)>;
    bool post(Command command);
    void run(std::unique_ptr<ICameraDevice> cameraDevice);

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<Command> m_commands;
    std::atomic<State> m_state{State::Closed};
    bool m_shutdownRequested{false};

    std::thread m_controlThread;

};



} // learnopengl::application
