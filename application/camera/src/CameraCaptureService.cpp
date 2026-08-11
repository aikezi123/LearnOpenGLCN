#include <camera/CameraCaptureService.h>
#include <stdexcept>
#include <utility>

namespace learnopengl::application {

CameraCaptureService::CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice) {
    if (cameraDevice == nullptr) {
        throw std::invalid_argument("require a camera device");
    }

    m_controlThread = std::thread(&CameraCaptureService::run, this, std::move(cameraDevice));
}

CameraCaptureService::~CameraCaptureService() {
    shutdown();
}

bool CameraCaptureService::post(Command command) {
    if (command == nullptr) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_shutdownRequested == true) {
            return false;
        }

        m_commands.push(std::move(command));
    }
    m_condition.notify_one();
    return true;

}

void CameraCaptureService::run(std::unique_ptr<ICameraDevice> cameraDevice) {

    while (1) {
        Command command;
        { // 开始临界区
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this]() {
                return !m_commands.empty() || m_shutdownRequested;
            });
            
            if (m_shutdownRequested && m_commands.empty()) {
                break;
            }
        
            command = std::move(m_commands.front());
            m_commands.pop();
        } // 退出临界区

        try {
            command(*cameraDevice);
        } catch(...) {

        }
    }

    // 线程退出，注销采集回调函数
    try {
        cameraDevice->setFrameCallback({});
    } catch(...) {

    }
    
    // 如果正在采集，先停止采集
    if (m_state.load() == State::Capturing) {
        try {
            CameraResult result = cameraDevice->stopCapture();
            if (result.succeeded) {
                m_state.store(State::Opened);
            }

        } catch(...) {

        }
    }

    // 如果没有关闭，则关闭设备
    if (m_state.load() != State::Closed) {
        try {
            CameraResult result = cameraDevice->close();
            if (result.succeeded) {
                m_state.store(State::Closed);
            }
        } catch(...) {

        }
    }

}


void CameraCaptureService::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdownRequested = true;
    }
    m_condition.notify_one();

    if (m_controlThread.joinable()) {
        m_controlThread.join();
    }
}

CameraCaptureService::State CameraCaptureService::state() const noexcept {
    return m_state;
}

bool CameraCaptureService::requestOpen() {
    return post([this](ICameraDevice &cameraDevice) {
        // 已经打开或正在录制，不需要再继续打开
        if (m_state.load() == State::Opened || m_state.load() == State::Capturing) {
            return;
        }

        CameraResult result = cameraDevice.openFirstCamera();
        if (result.succeeded) {
            m_state.store(State::Opened);
        }
    });
}
bool CameraCaptureService::requestClose() {
    return post([this](ICameraDevice &cameraDevice) {
        // 如果已经关闭，则不需要关闭
        if (m_state.load() == State::Closed) {
            return;
        }

        // 如果正在采集，停止采集
        if (m_state.load() == State::Capturing) {
            CameraResult result = cameraDevice.stopCapture();
            if (!result.succeeded) {
                return;
            }
            m_state.store(State::Opened);
        }

        // 如果正在打开则关闭
        if (m_state.load() == State::Opened) {
            CameraResult result = cameraDevice.close();
            if (result.succeeded) {
                m_state.store(State::Closed);
            }
        }
    });
}
bool CameraCaptureService::requestStartCapture() {
    return post([this](ICameraDevice &cameraDevice) {
        if (m_state.load() != State::Opened) {
            return;
        }
        CameraResult result = cameraDevice.startCapture();
        if (result.succeeded) {
            m_state.store(State::Capturing);
        }
    });
}
bool CameraCaptureService::requestStopCapture() {
    return post([this](ICameraDevice &cameraDevice) {
        if (m_state.load() != State::Capturing) {
            return;
        }
        CameraResult result = cameraDevice.stopCapture();
        if (result.succeeded) {
            m_state.store(State::Opened);
        }
    });
}
bool CameraCaptureService::requestSetFrameCallback(ICameraDevice::FrameCallback callback) {
    return post([callback = std::move(callback)](ICameraDevice &cameraDevice) mutable {
        cameraDevice.setFrameCallback(std::move(callback));
    });
}


} // namespace learnopengl::application
