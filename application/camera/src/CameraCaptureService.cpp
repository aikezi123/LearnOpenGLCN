#include <camera/CameraCaptureService.h>

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
        try{
            result = command.action(*cameraDevice);
            command.promiseResult.set_value(std::move(result));   // promise异步修改命令执行结果
        } catch(...) {

        }

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