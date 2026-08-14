#include <camera/CameraCaptureService.h>

#include <cmath>
#include <exception>
#include <stdexcept>
#include <utility>

namespace learnopengl::application {

namespace {

CameraResult succeed() {
    CameraResult result;
    result.succeeded = true;
    return result;
}

CameraResult failure(std::string errorMsg) {
    CameraResult result;
    result.succeeded = false;
    result.errorMessage = errorMsg;
    return result;
}

} // namespace

CameraCaptureService::CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice) {
    if (cameraDevice == nullptr) {
        throw std::invalid_argument("构造函数无有效传递的相机设备对象");
    }
    
    // 构造函数中直接启动控制线程
    m_thread = std::thread(&CameraCaptureService::run, this, std::move(cameraDevice));
}

CameraCaptureService::~CameraCaptureService() {
    shutdown();
}

void CameraCaptureService::shutdown() {
    // 请求停止状态置为true，并唤醒条件变量让线程准备退出
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdownRequest = true;
    }
    m_condition.notify_one();

    // 等待线程结束
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

std::future<CameraResult> CameraCaptureService::submit(CommandAction action) {
    // Command中包含命令以及std::promise<CameraResult>,需要在这里把promise和future建立共享关系。
    Command command;
    std::future<CameraResult> future = command.promiseResult.get_future();

    if (action == nullptr) {
        command.promiseResult.set_value(failure("命令为空指针"));
        return future;
    }
    command.action = std::move(action);

    // 准备将命令CommandAction投递到队列中，并唤醒控制线程进行处理
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 如果正在请求退出，则拒绝任何命令的提交
        if (m_shutdownRequest == true) {
            command.promiseResult.set_value(failure("正在请求退出，拒绝任何命令的加入"));
            return future;
        }
        m_commands.push(std::move(command));
    }
    m_condition.notify_one();
    return future;
}

void CameraCaptureService::run(std::unique_ptr<ICameraDevice> cameraDevice) {
    while (1) {
        Command command;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // 只有请求退出或者命令队列不为空时唤醒
            m_condition.wait(lock, [this]() {
                return m_shutdownRequest || !m_commands.empty();
            });

            // 只有请求退出，并且命令队列处理完之后才退出循环。
            if (m_shutdownRequest && m_commands.empty()) {
                break;
            }
            command = std::move(m_commands.front());
            m_commands.pop();
        }

        CameraResult result;
        try {
            result = command.action(*cameraDevice);
        }
        catch (const std::exception& exception) {
            // 设备操作抛出标准异常时，将异常转换成普通失败结果。
            // 这样调用方仍然可以通过future.get()得到CameraResult，
            // 不会因为promise未完成而收到broken_promise。
            result = failure(std::string("执行相机命令时发生异常：") + exception.what()
            );
        }
        catch (...) {
            // 第三方SDK也可能抛出非std::exception异常。
            result = failure("执行相机命令时发生未知异常");
        }

        // 每一条已经从队列取出的命令都只在这里完成一次promise。
        command.promiseResult.set_value(std::move(result));
    }
    
    // 此时请求退出，并且任务队列中的任务已经处理完成，关闭相机设备
    try {
        cameraDevice->setFrameCallback(ICameraDevice::FrameCallback{});
        cameraDevice->stopCapture();
        cameraDevice->close();
    } catch(...) {

    }
}

CameraCaptureService::State CameraCaptureService::state() const {
    return  m_state.load();
}

std::future<CameraResult> CameraCaptureService::requestOpen() {
    return submit([this](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() != State::Closed) {
            return failure("相机已打开，忽略重复打开命令");
        }
        CameraResult result = cameraDevice.openFirstCamera();
        if (result.succeeded) {
            m_state.store(State::Opened);;
        }
        return result;
    });
}

std::future<CameraResult> CameraCaptureService::requestOpenById(std::string deviceId) {
    return submit([this, deviceId = std::move(deviceId)](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() != State::Closed) {
            return failure("相机已打开，忽略重复打开命令");
        }
        if (deviceId.empty()) {
            return failure("相机ID不能为空");
        }

        CameraResult result = cameraDevice.openById(deviceId);
        if (result.succeeded) {
            m_state.store(State::Opened);
        }
        return result;
    });
}

std::future<CameraResult> CameraCaptureService::requestOpenByName(std::string deviceName) {
    return submit([this, deviceName = std::move(deviceName)](ICameraDevice &cameraDevice) mutable ->CameraResult {
        if (m_state.load() != State::Closed) {
            return failure("相机已打开，忽略重复打开命令");
        }
        if (deviceName.empty()) {
            return failure("相机名称不能为空");
        }

        CameraResult result = cameraDevice.openCameraByName(std::move(deviceName));
        if (result.succeeded) {
            m_state.store(State::Opened);
        }
        return result;
    });
}

std::future<CameraResult> CameraCaptureService::requestClose() {
    return submit([this](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() == State::Closed) {
            return failure("相机已关闭，忽略重复关闭命令");
        }
        if (m_state.load() == State::Captured) {
            CameraResult result = cameraDevice.stopCapture();
            if (result.succeeded) {
                m_state.store(State::Opened);
            } else {
                return failure("关闭相机时无法停止捕获流");
            }
        }
        CameraResult result = cameraDevice.close();
        if (result.succeeded) {
            m_state.store(State::Closed);
        }
        return result;
    });
}

std::future<CameraResult> CameraCaptureService::requestStartCapture() {
    return submit([this](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() == State::Closed) {
            return failure("打开帧捕获流失败，请先打开相机");
        }
        if (m_state.load() == State::Captured) {
            return failure("帧捕获流已经打开，忽略重复指令");
        }
        CameraResult result = cameraDevice.startCapture();
        if (result.succeeded) {
            m_state = State::Captured;
        }
        return result;
    });
}

std::future<CameraResult> CameraCaptureService::requestStopCaptrue() {
    return submit([this](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() != State::Captured) {
            return failure("关闭帧捕获流失败，帧捕获流已停止");
        }
        CameraResult result = cameraDevice.stopCapture();
        if (result.succeeded) {
            m_state.store(State::Opened);
        }
        return result;
    });
}

std::future<CameraResult> CameraCaptureService::requestSetAutoWhiteBalance(bool enable) {
    return submit([this, enable](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() == State::Closed) {
            return failure("设置自动白平衡失败，请先打开相机");
        }
        return cameraDevice.setAutoWhiteBalance(enable);
    });
}

std::future<CameraResult> CameraCaptureService::requestSetExposeTimeUs(double usTime) {
    return submit([this, usTime](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() == State::Closed) {
            return failure("设置曝光时间失败，请先打开相机");
        }
        if (!std::isfinite(usTime) || usTime <= 0.0) {
            return failure("曝光时间必须是大于0的有限数值");
        }
        return cameraDevice.setExposeTimeUs(usTime);
    });
}

std::future<CameraResult> CameraCaptureService::requestSetGainDb(double gain) {
    return submit([this, gain](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() == State::Closed) {
            return failure("设置增益失败，请先打开相机");
        }
        if (!std::isfinite(gain)) {
            return failure("增益必须是有限数值");
        }
        return cameraDevice.setGainDb(gain);
    });
}

std::future<CameraResult> CameraCaptureService::requestSetFps(double fps) {
    return submit([this, fps](ICameraDevice &cameraDevice)->CameraResult {
        if (m_state.load() == State::Closed) {
            return failure("设置帧率失败，请先打开相机");
        }
        if (!std::isfinite(fps) || fps <= 0.0) {
            return failure("帧率必须是大于0的有限数值");
        }
        return cameraDevice.setFps(fps);
    });
}

std::future<CameraResult> CameraCaptureService::requestSetFrameCallback(ICameraDevice::FrameCallback callback) {
        return submit([callback = std::move(callback)](ICameraDevice& cameraDevice) mutable {
            /*
             * ICameraDevice::setFrameCallback()返回void。
             *
             * 如果调用过程没有抛出异常，就由Application
             * 构造一个成功的CameraResult。
             */
            cameraDevice.setFrameCallback(
                std::move(callback)
            );

            return succeed();
        }
    );
}


} //learnopengl::application
