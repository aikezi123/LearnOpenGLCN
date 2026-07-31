#include <camera/galaxy/GalaxyCameraController.h>

// Galaxy SDK 仅在 Infrastructure 实现文件中使用，避免向上层泄漏厂商类型。
#include <GalaxyIncludes.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <limits>
#include <mutex>
#include <utility>

namespace learnopengl::infrastructure::camera::galaxy {
namespace {

std::string makeGalaxyError(const char* operation, CGalaxyException& error)
{
    return std::string(operation)
        + " failed. code="
        + std::to_string(error.GetErrorCode())
        + ", info="
        + error.what();
}

std::string makeUnknownError(const char* operation)
{
    return std::string(operation) + " failed with unknown error.";
}

bool calculateRgb24Size(uint64_t width, uint64_t height, size_t& byteCount)
{
    if (width == 0 || height == 0) {
        return false;
    }

    const auto maxInt = static_cast<uint64_t>((std::numeric_limits<int>::max)());
    if (width > maxInt || height > maxInt) {
        return false;
    }

    constexpr uint64_t kChannels = 3;
    if (width > (std::numeric_limits<uint64_t>::max)() / height) {
        return false;
    }

    const uint64_t pixelCount = width * height;
    if (pixelCount > (std::numeric_limits<uint64_t>::max)() / kChannels) {
        return false;
    }

    const uint64_t bytes = pixelCount * kChannels;
    if (bytes > static_cast<uint64_t>((std::numeric_limits<size_t>::max)())) {
        return false;
    }

    byteCount = static_cast<size_t>(bytes);
    return true;
}

} // namespace

class GalaxyCameraControllerImpl;

class GalaxyCaptureHandler final : public ICaptureEventHandler {
public:
    explicit GalaxyCaptureHandler(GalaxyCameraControllerImpl* controller);
    void DoOnImageCaptured(CImageDataPointer& imageData, void* userParam) override;

private:
    GalaxyCameraControllerImpl* m_controller{nullptr};
};

class GalaxyCameraControllerImpl final {
public:
    using FrameCallback = application::ICameraDevice::FrameCallback;

    GalaxyCameraControllerImpl() = default;
    ~GalaxyCameraControllerImpl();

    GalaxyCameraControllerImpl(const GalaxyCameraControllerImpl&) = delete;
    GalaxyCameraControllerImpl& operator=(const GalaxyCameraControllerImpl&) = delete;

    bool openFirstCamera();
    bool openBySerialNumber(const std::string& serialNumber);
    bool openByUserId(const std::string& userId);
    bool startGrabbing();
    bool stopGrabbing();
    bool close();

    bool setAutoWhiteBalance(bool enabled);
    bool setGain(double value);
    bool setExposureTime(double value);
    bool setFps(double value);

    bool isOpen() const;
    bool isGrabbing() const;
    std::string lastError() const;
    application::CameraResult makeResult(bool succeeded) const;
    void setFrameCallback(FrameCallback callback);
    void handleFrame(CImageDataPointer& imageData);

private:
    bool initializeSdk();
    void uninitializeSdk();
    void cleanupStream();
    void setLastError(std::string message);

    CGXDevicePointer m_device;
    CGXStreamPointer m_stream;
    CGXFeatureControlPointer m_featureControl;
    std::unique_ptr<GalaxyCaptureHandler> m_captureHandler;

    FrameCallback m_frameCallback;
    mutable std::mutex m_callbackMutex;
    mutable std::mutex m_errorMutex;
    std::string m_lastError;

    bool m_sdkInitialized{false};
    std::atomic_bool m_isOpen{false};
    std::atomic_bool m_isGrabbing{false};
};

GalaxyCaptureHandler::GalaxyCaptureHandler(GalaxyCameraControllerImpl* controller)
    : m_controller(controller)
{
}

void GalaxyCaptureHandler::DoOnImageCaptured(CImageDataPointer& imageData, void*)
{
    if (m_controller != nullptr) {
        m_controller->handleFrame(imageData);
    }
}

GalaxyCameraControllerImpl::~GalaxyCameraControllerImpl()
{
    static_cast<void>(close());
}

bool GalaxyCameraControllerImpl::openFirstCamera()
{
    if (m_isOpen.load()) {
        return true;
    }

    if (!initializeSdk()) {
        return false;
    }

    try {
        GxIAPICPP::gxdeviceinfo_vector devices;
        IGXFactory::GetInstance().UpdateAllDeviceList(100, devices);

        if (devices.empty()) {
            setLastError("No Galaxy camera was found.");
            uninitializeSdk();
            return false;
        }

        const CGXDeviceInfo& deviceInfo = devices.front();
        const GxIAPICPP::gxstring userId = deviceInfo.GetUserID();

        if (!userId.empty()) {
            m_device = IGXFactory::GetInstance().OpenDeviceByUserID(userId, GX_ACCESS_EXCLUSIVE);
        }
        else {
            const GxIAPICPP::gxstring serialNumber = deviceInfo.GetSN();
            if (serialNumber.empty()) {
                setLastError("The first Galaxy camera has neither UserID nor serial number.");
                uninitializeSdk();
                return false;
            }

            m_device = IGXFactory::GetInstance().OpenDeviceBySN(serialNumber, GX_ACCESS_EXCLUSIVE);
        }

        m_featureControl = m_device->GetRemoteFeatureControl();
        m_isOpen.store(true);
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Open first Galaxy camera", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Open first Galaxy camera"));
    }

    m_device = CGXDevicePointer();
    uninitializeSdk();
    return false;
}

bool GalaxyCameraControllerImpl::openBySerialNumber(
    const std::string& serialNumber
)
{
    if (m_isOpen.load()) {
        return true;
    }

    if (serialNumber.empty()) {
        setLastError("Galaxy camera serial number is empty.");
        return false;
    }

    if (!initializeSdk()) {
        return false;
    }

    try {
        m_device = IGXFactory::GetInstance().OpenDeviceBySN(
            serialNumber.c_str(),
            GX_ACCESS_EXCLUSIVE
        );
        m_featureControl = m_device->GetRemoteFeatureControl();
        m_isOpen.store(true);
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Open Galaxy camera by serial number", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Open Galaxy camera by serial number"));
    }

    m_featureControl = CGXFeatureControlPointer();
    m_device = CGXDevicePointer();
    uninitializeSdk();
    return false;
}

bool GalaxyCameraControllerImpl::openByUserId(const std::string& userId)
{
    if (m_isOpen.load()) {
        return true;
    }

    if (userId.empty()) {
        setLastError("Galaxy camera UserID is empty.");
        return false;
    }

    if (!initializeSdk()) {
        return false;
    }

    try {
        m_device = IGXFactory::GetInstance().OpenDeviceByUserID(userId.c_str(), GX_ACCESS_EXCLUSIVE);
        m_featureControl = m_device->GetRemoteFeatureControl();
        m_isOpen.store(true);
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Open Galaxy camera by UserID", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Open Galaxy camera by UserID"));
    }

    m_device = CGXDevicePointer();
    uninitializeSdk();
    return false;
}

bool GalaxyCameraControllerImpl::startGrabbing()
{
    if (m_isGrabbing.load()) {
        return true;
    }

    if (!m_isOpen.load() || m_device.IsNull()) {
        setLastError("Galaxy camera is not open.");
        return false;
    }

    try {
        if (m_device->GetStreamCount() == 0) {
            setLastError("Galaxy camera has no stream.");
            return false;
        }

        if (m_featureControl.IsNull()) {
            m_featureControl = m_device->GetRemoteFeatureControl();
        }

        m_stream = m_device->OpenStream(0);
        m_captureHandler = std::make_unique<GalaxyCaptureHandler>(this);

        m_stream->RegisterCaptureCallback(m_captureHandler.get(), this);
        m_stream->StartGrab();
        m_featureControl->GetCommandFeature("AcquisitionStart")->Execute();

        m_isGrabbing.store(true);
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Start Galaxy camera grabbing", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Start Galaxy camera grabbing"));
    }

    cleanupStream();
    return false;
}

bool GalaxyCameraControllerImpl::stopGrabbing()
{
    cleanupStream();
    return true;
}

bool GalaxyCameraControllerImpl::close()
{
    cleanupStream();
    bool succeeded = true;

    if (!m_device.IsNull()) {
        try {
            m_device->Close();
        }
        catch (CGalaxyException& error) {
            setLastError(makeGalaxyError("Close Galaxy camera", error));
            succeeded = false;
        }
        catch (...) {
            setLastError(makeUnknownError("Close Galaxy camera"));
            succeeded = false;
        }
    }

    m_featureControl = CGXFeatureControlPointer();
    m_device = CGXDevicePointer();
    m_isOpen.store(false);
    uninitializeSdk();
    return succeeded;
}

bool GalaxyCameraControllerImpl::setAutoWhiteBalance(bool enabled)
{
    if (!m_isOpen.load() || m_featureControl.IsNull()) {
        setLastError("Cannot set Galaxy auto white balance: camera is not open.");
        return false;
    }

    try {
        if (!m_featureControl->IsImplemented("BalanceWhiteAuto")) {
            setLastError("Galaxy camera does not support auto white balance.");
            return false;
        }

        auto whiteBalanceFeature = m_featureControl->GetEnumFeature("BalanceWhiteAuto");
        whiteBalanceFeature->SetValue(enabled ? "Continuous" : "Off");
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Set Galaxy camera auto white balance", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Set Galaxy camera auto white balance"));
    }

    return false;
}

bool GalaxyCameraControllerImpl::setGain(double value)
{
    if (!m_isOpen.load() || m_featureControl.IsNull()) {
        setLastError("Cannot set Galaxy gain: camera is not open.");
        return false;
    }

    try {
        if (!m_featureControl->IsImplemented("Gain")) {
            setLastError("Galaxy camera does not support gain control.");
            return false;
        }

        auto gainFeature = m_featureControl->GetFloatFeature("Gain");

        const double min = gainFeature->GetMin();
        const double max = gainFeature->GetMax();

        if (value < min) {
            value = min;
        }

        if (value > max) {
            value = max;
        }

        gainFeature->SetValue(value);
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Set Galaxy camera gain", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Set Galaxy camera gain"));
    }

    return false;
}

bool GalaxyCameraControllerImpl::setExposureTime(double value)
{
    if (!m_isOpen.load() || m_featureControl.IsNull()) {
        setLastError("Cannot set Galaxy exposure time: camera is not open.");
        return false;
    }

    try {
        if (!m_featureControl->IsImplemented("ExposureTime")) {
            setLastError("Galaxy camera does not support exposure time control.");
            return false;
        }

        auto exposureFeature = m_featureControl->GetFloatFeature("ExposureTime");
        const double min = exposureFeature->GetMin();
        const double max = exposureFeature->GetMax();

        if (value < min) {
            value = min;
        }

        if (value > max) {
            value = max;
        }

        exposureFeature->SetValue(value);
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Set Galaxy camera exposure time", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Set Galaxy camera exposure time"));
    }

    return false;
}

bool GalaxyCameraControllerImpl::setFps(double value)
{
    if (!m_isOpen.load() || m_featureControl.IsNull()) {
        setLastError("Cannot set Galaxy frame rate: camera is not open.");
        return false;
    }

    try {
        if (!m_featureControl->IsImplemented("AcquisitionFrameRate")) {
            setLastError("Galaxy camera does not support frame rate control.");
            return false;
        }

        auto frameRateFeature =
            m_featureControl->GetFloatFeature("AcquisitionFrameRate");
        const double min = frameRateFeature->GetMin();
        const double max = frameRateFeature->GetMax();

        if (value < min) {
            value = min;
        }

        if (value > max) {
            value = max;
        }

        frameRateFeature->SetValue(value);
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Set Galaxy camera frame rate", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Set Galaxy camera frame rate"));
    }

    return false;
}

bool GalaxyCameraControllerImpl::isOpen() const
{
    return m_isOpen.load();
}

bool GalaxyCameraControllerImpl::isGrabbing() const
{
    return m_isGrabbing.load();
}

std::string GalaxyCameraControllerImpl::lastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

application::CameraResult GalaxyCameraControllerImpl::makeResult(
    bool succeeded
) const
{
    return application::CameraResult{
        succeeded,
        succeeded ? std::string{} : lastError()
    };
}

void GalaxyCameraControllerImpl::setFrameCallback(FrameCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_frameCallback = std::move(callback);
}

bool GalaxyCameraControllerImpl::initializeSdk()
{
    if (m_sdkInitialized) {
        return true;
    }

    try {
        IGXFactory::GetInstance().Init();
        m_sdkInitialized = true;
        return true;
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Initialize Galaxy SDK", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Initialize Galaxy SDK"));
    }

    return false;
}

void GalaxyCameraControllerImpl::uninitializeSdk()
{
    if (!m_sdkInitialized) {
        return;
    }

    try {
        IGXFactory::GetInstance().Uninit();
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Uninitialize Galaxy SDK", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Uninitialize Galaxy SDK"));
    }

    m_sdkInitialized = false;
}

void GalaxyCameraControllerImpl::cleanupStream()
{
    if (!m_featureControl.IsNull()) {
        try {
            m_featureControl->GetCommandFeature("AcquisitionStop")->Execute();
        }
        catch (...) {
        }
    }

    if (!m_stream.IsNull()) {
        try {
            m_stream->StopGrab();
        }
        catch (...) {
        }

        try {
            m_stream->UnregisterCaptureCallback();
        }
        catch (...) {
        }

        try {
            m_stream->Close();
        }
        catch (...) {
        }
    }

    m_captureHandler.reset();
    m_stream = CGXStreamPointer();
    m_isGrabbing.store(false);
}

void GalaxyCameraControllerImpl::handleFrame(CImageDataPointer& imageData)
{
    if (imageData.IsNull() || imageData->GetStatus() != GX_FRAME_STATUS_SUCCESS) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (!m_frameCallback) {
            return;
        }
    }

    const uint64_t width = imageData->GetWidth();
    const uint64_t height = imageData->GetHeight();

    size_t byteCount = 0;
    if (!calculateRgb24Size(width, height, byteCount)) {
        setLastError("Galaxy frame size is invalid.");
        return;
    }

    void* rgbBuffer = imageData->ConvertToRGB24(
        GX_BIT_0_7,
        GX_RAW2RGB_NEIGHBOUR,
        true
    );

    if (rgbBuffer == nullptr) {
        setLastError("Convert Galaxy frame to RGB24 failed.");
        return;
    }

    domain::ImageFrame frame;
    frame.width = static_cast<int>(width);
    frame.height = static_cast<int>(height);
    frame.frameId = imageData->GetFrameID();
    frame.pixelFormat = domain::PixelFormat::Rgb24;
    frame.pixels.resize(byteCount);

    std::memcpy(frame.pixels.data(), rgbBuffer, byteCount);

    // 回调调用也处于同一把锁的保护下。setFrameCallback({}) 返回时，
    // 可以确定此前进入的回调已经结束，避免 UI 析构期间发生释放后访问。
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (m_frameCallback) {
            m_frameCallback(std::move(frame));
        }
    }
}

void GalaxyCameraControllerImpl::setLastError(std::string message)
{
    const std::string text = std::move(message);
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError = text;
    }

    std::cerr << text << std::endl;
}

GalaxyCameraController::GalaxyCameraController()
    : m_impl(std::make_unique<GalaxyCameraControllerImpl>())
{
}

GalaxyCameraController::~GalaxyCameraController() = default;

application::CameraResult GalaxyCameraController::openFirstCamera()
{
    const bool succeeded = m_impl->openFirstCamera();
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::openById(
    const std::string& deviceId
)
{
    const bool succeeded = m_impl->openBySerialNumber(deviceId);
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::openCameraByName(
    std::string deviceName
)
{
    const bool succeeded = m_impl->openByUserId(deviceName);
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::startCapture()
{
    const bool succeeded = m_impl->startGrabbing();
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::stopCapture()
{
    const bool succeeded = m_impl->stopGrabbing();
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::close()
{
    const bool succeeded = m_impl->close();
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::setAutoWhiteBalance(
    bool enable
)
{
    const bool succeeded = m_impl->setAutoWhiteBalance(enable);
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::setExposeTimeUs(
    double usTime
)
{
    const bool succeeded = m_impl->setExposureTime(usTime);
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::setGainDb(double gain)
{
    const bool succeeded = m_impl->setGain(gain);
    return m_impl->makeResult(succeeded);
}

application::CameraResult GalaxyCameraController::setFps(double fps)
{
    const bool succeeded = m_impl->setFps(fps);
    return m_impl->makeResult(succeeded);
}

void GalaxyCameraController::setFrameCallback(FrameCallback callback)
{
    m_impl->setFrameCallback(std::move(callback));
}

} // namespace learnopengl::infrastructure::camera::galaxy
