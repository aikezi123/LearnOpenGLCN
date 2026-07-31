#include <camera/CameraCaptureService.h>
#include <stdexcept>
#include <utility>

namespace learnopengl::application {

    CameraCaptureService::CameraCaptureService(std::unique_ptr<ICameraDevice> cameraDevice) {
        m_cameraDevice = std::move(cameraDevice);

        if (m_cameraDevice == nullptr) {
            throw std::invalid_argument("CameraModule: CameraCaptureService requires a camera device");
        }
    }

    CameraCaptureService::CameraCaptureService(CameraCaptureService&&) noexcept = default;

    CameraCaptureService& CameraCaptureService::operator=(
        CameraCaptureService&&
    ) noexcept = default;

    CameraResult CameraCaptureService::openFirstCamera() {
        return m_cameraDevice->openFirstCamera();
    }
    CameraResult CameraCaptureService::openCameraById(std::string deviceId) {
        return m_cameraDevice->openById(deviceId);
    }
    CameraResult CameraCaptureService::openCameraByName(std::string deviceName) {
        return m_cameraDevice->openCameraByName(deviceName);
    }
    CameraResult CameraCaptureService::startCapture() {
        return m_cameraDevice->startCapture();
    }
    CameraResult CameraCaptureService::stopCapture() {
        return m_cameraDevice->stopCapture();
    }
    CameraResult CameraCaptureService::close() {
        return m_cameraDevice->close();
    }

    CameraResult CameraCaptureService::setAutoWhiteBalance(bool enable) {
        return m_cameraDevice->setAutoWhiteBalance(enable);
    }
    CameraResult CameraCaptureService::setExposeTimeUs(double usTime) {
        return m_cameraDevice->setExposeTimeUs(usTime);
    }
    CameraResult CameraCaptureService::setGainDb(double gain) {
        return m_cameraDevice->setGainDb(gain);
    }
    CameraResult CameraCaptureService::setFps(double fps) {
        return m_cameraDevice->setFps(fps);
    }

    void CameraCaptureService::setFrameCallback(ICameraDevice::FrameCallback callback) {
        m_cameraDevice->setFrameCallback(std::move(callback));
    }

} // namespace learnopengl::application
