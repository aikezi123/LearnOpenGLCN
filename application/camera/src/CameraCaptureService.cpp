#include <camera/CameraCaptureService.h>
#include <stdexcept>
#include <utility>

namespace learnopengl::application {

    CameraCaptureService::CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice) {
        if (cameraDevice == nullptr) {
            throw std::invalid_argument("CameraCaptureService requires a camera device");
        }

        // std::move 将 cameraDevice 的所有权交给新线程
        // 最终新线程会调用run(std::unique_ptr<ICameraDevice> cameraDevice)
        m_controlThread = std::thread(&CameraCaptureService::run, this, std::move(cameraDevice));
    }

    CameraCaptureService::~CameraCaptureService() {
        { 
            std::lock_guard<std::mutex> lock(m_mutex);

            // 修改等待条件
            m_shutdownRequested = false;
        }
        
        // 唤醒可能正在 condition_variable::wait()中休眠的控制线程
        m_conditionVariable.notify_one();

        // std::thread析构前必须完成join，保证service销毁前，线程已经销毁
        if (m_controlThread.joinable()) {
            m_controlThread.join();
        }

    }

    void CameraCaptureService::run(std::unique_ptr<ICameraDevice> cameraDevice) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            // wait(lock, predicate)
            m_conditionVariable.wait(lock, [this]()->bool {
                return m_shutdownRequested;
            });
        }


    }

    CameraCaptureService::CameraCaptureService(CameraCaptureService&&) noexcept = default;

    CameraCaptureService& CameraCaptureService::operator=(CameraCaptureService&&) noexcept = default;

    CameraResult CameraCaptureService::openFirstCamera() {

    }
    CameraResult CameraCaptureService::openCameraById(std::string deviceId) {

    }
    CameraResult CameraCaptureService::openCameraByName(std::string deviceName) {

    }
    CameraResult CameraCaptureService::startCapture() {

    }
    CameraResult CameraCaptureService::stopCapture() {

    }
    CameraResult CameraCaptureService::close() {
  
    }

    CameraResult CameraCaptureService::setAutoWhiteBalance(bool enable) {

    }
    CameraResult CameraCaptureService::setExposeTimeUs(double usTime) {

    }
    CameraResult CameraCaptureService::setGainDb(double gain) {

    }
    CameraResult CameraCaptureService::setFps(double fps) {

    }

    void CameraCaptureService::setFrameCallback(ICameraDevice::FrameCallback callback) {

    }

} // namespace learnopengl::application
