#include <camera/CameraPreviewService.h>

#include <utility>

namespace learnopengl::application {

CameraPreviewService::CameraPreviewService(std::unique_ptr<ICameraDevice> cameraDevice)
    : m_cameraDevice(std::move(cameraDevice))
{
}

CameraPreviewService::~CameraPreviewService()
{
    close();
}

bool CameraPreviewService::startPreview()
{
    if (m_cameraDevice == nullptr) {
        return false;
    }

    if (!m_cameraDevice->openFirstCamera()) {
        return false;
    }

    return m_cameraDevice->startGrabbing();
}

bool CameraPreviewService::startPreviewByUserId(const std::string& userId)
{
    if (m_cameraDevice == nullptr) {
        return false;
    }

    if (!m_cameraDevice->openByUserId(userId)) {
        return false;
    }

    return m_cameraDevice->startGrabbing();
}

void CameraPreviewService::stopPreview()
{
    if (m_cameraDevice != nullptr) {
        m_cameraDevice->stopGrabbing();
    }
}

void CameraPreviewService::close()
{
    if (m_cameraDevice != nullptr) {
        m_cameraDevice->close();
    }
}

void CameraPreviewService::setAutoWhiteBalance(bool enabled)
{
    if (m_cameraDevice != nullptr) {
        m_cameraDevice->setAutoWhiteBalance(enabled);
    }
}

void CameraPreviewService::setGain(double value)
{
    if (m_cameraDevice != nullptr) {
        m_cameraDevice->setGain(value);
    }
}

void CameraPreviewService::setExposureTime(double value)
{
    if (m_cameraDevice != nullptr) {
        m_cameraDevice->setExposureTime(value);
    }
}

bool CameraPreviewService::isOpen() const
{
    return m_cameraDevice != nullptr && m_cameraDevice->isOpen();
}

bool CameraPreviewService::isGrabbing() const
{
    return m_cameraDevice != nullptr && m_cameraDevice->isGrabbing();
}

std::string CameraPreviewService::lastError() const
{
    if (m_cameraDevice == nullptr) {
        return "Camera device is not configured.";
    }

    return m_cameraDevice->lastError();
}

void CameraPreviewService::setFrameCallback(ICameraDevice::FrameCallback callback)
{
    if (m_cameraDevice != nullptr) {
        m_cameraDevice->setFrameCallback(std::move(callback));
    }
}

} // namespace learnopengl::application
