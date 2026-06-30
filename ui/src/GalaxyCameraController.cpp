#include "GalaxyCameraController.h"

#include <cstring>
#include <iostream>
#include <limits>
#include <utility>

namespace learnopengl::ui {
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
    // RGB24 每个像素 3 字节。这里先做溢出检查，避免异常尺寸导致 vector 分配错误。
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

GalaxyCaptureHandler::GalaxyCaptureHandler(GalaxyCameraController* controller)
    : m_controller(controller)
{
}

void GalaxyCaptureHandler::DoOnImageCaptured(
    CImageDataPointer& imageData,
    void*)
{
    // SDK 在采集线程回调这里。不要在本函数里做 OpenGL 操作，只转给控制器处理帧数据。
    if (m_controller != nullptr) {
        m_controller->handleFrame(imageData);
    }
}

GalaxyCameraController::GalaxyCameraController() = default;

GalaxyCameraController::~GalaxyCameraController()
{
    close();
}

bool GalaxyCameraController::openFirstCamera()
{
    if (m_isOpen.load()) {
        return true;
    }

    if (!initializeSdk()) {
        return false;
    }

    try {
        // 先枚举设备，再打开第一台。若相机配置过 UserID，优先按 UserID 打开；
        // 否则退回用序列号打开，方便未配置 UserID 的相机也能在原型阶段跑起来。
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

bool GalaxyCameraController::openByUserId(const std::string& userId)
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

bool GalaxyCameraController::startGrabbing()
{
    if (m_isGrabbing.load()) {
        return true;
    }

    if (!m_isOpen.load() || m_device.IsNull()) {
        setLastError("Galaxy camera is not open.");
        return false;
    }

    try {
        // 当前只使用第 0 路数据流。大恒 SDK 收到图像后会调用 GalaxyCaptureHandler。
        if (m_device->GetStreamCount() == 0) {
            setLastError("Galaxy camera has no stream.");
            return false;
        }

        m_featureControl = m_device->GetRemoteFeatureControl();
        setExposureTime(6000);
        setGain(2);
        setAutoWhiteBalance(true);

        m_stream = m_device->OpenStream(0);
        m_captureHandler = std::make_unique<GalaxyCaptureHandler>(this);

        m_stream->RegisterCaptureCallback(m_captureHandler.get(), this);
        m_stream->StartGrab();
        // StartGrab 启动本地流对象；AcquisitionStart 才是真正让相机开始出图。
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

void GalaxyCameraController::stopGrabbing()
{
    cleanupStream();
}

void GalaxyCameraController::close()
{
    cleanupStream();

    if (!m_device.IsNull()) {
        try {
            m_device->Close();
        }
        catch (CGalaxyException& error) {
            setLastError(makeGalaxyError("Close Galaxy camera", error));
        }
        catch (...) {
            setLastError(makeUnknownError("Close Galaxy camera"));
        }
    }

    m_device = CGXDevicePointer();
    m_isOpen.store(false);
    uninitializeSdk();
}

void GalaxyCameraController::setAutoWhiteBalance(bool flag)
{
    if (!m_isOpen.load() || m_featureControl.IsNull()) {
        return;
    }

    try {
        if (!m_featureControl->IsImplemented("BalanceWhiteAuto")) {
            return;
        }

        auto whiteBalanceFeature = m_featureControl->GetEnumFeature("BalanceWhiteAuto");
        whiteBalanceFeature->SetValue(flag ? "Continuous" : "Off");
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Set Galaxy camera auto white balance", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Set Galaxy camera auto white balance"));
    }
}

void GalaxyCameraController::setGain(double value)
{
    if (!m_isOpen.load() || m_featureControl.IsNull()) {
        return;
    }

    try {
        if (!m_featureControl->IsImplemented("Gain")) {
            return;
        }

        auto gainFeature = m_featureControl->GetFloatFeature("Gain");

        double min = gainFeature->GetMin();
        double max = gainFeature->GetMax();

        if (value < min) {
            value = min;
        }

        if (value > max) {
            value = max;
        }

        gainFeature->SetValue(value);
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Set Galaxy camera gain", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Set Galaxy camera gain"));
    }

}

void GalaxyCameraController::setExposureTime(double value)
{
    if (!m_isOpen.load() || m_featureControl.IsNull()) {
        return;
    }
    try {
        if (!m_featureControl->IsImplemented("ExposureTime")) {
            return;
        }

        auto exposureFeature = m_featureControl->GetFloatFeature("ExposureTime");
        double min = exposureFeature->GetMin();
        double max = exposureFeature->GetMax();

        if (value < min) {
            value = min;
        }

        if (value > max) {
            value = max;
        }

        exposureFeature->SetValue(value);
    }
    catch (CGalaxyException& error) {
        setLastError(makeGalaxyError("Set Galaxy camera exposure time", error));
    }
    catch (...) {
        setLastError(makeUnknownError("Set Galaxy camera exposure time"));
    }

}

bool GalaxyCameraController::isOpen() const
{
    return m_isOpen.load();
}

bool GalaxyCameraController::isGrabbing() const
{
    return m_isGrabbing.load();
}

std::string GalaxyCameraController::lastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

void GalaxyCameraController::setFrameCallback(FrameCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_frameCallback = std::move(callback);
}

bool GalaxyCameraController::initializeSdk()
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

void GalaxyCameraController::uninitializeSdk()
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

void GalaxyCameraController::cleanupStream()
{
    // 关闭顺序和启动顺序相反：先让相机停止出图，再停本地流、注销回调、关闭流。
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
    m_featureControl = CGXFeatureControlPointer();
    m_stream = CGXStreamPointer();
    m_isGrabbing.store(false);
}

void GalaxyCameraController::handleFrame(CImageDataPointer& imageData)
{
    if (imageData.IsNull() || imageData->GetStatus() != GX_FRAME_STATUS_SUCCESS) {
        return;
    }

    FrameCallback callback;
    {
        // 复制一份 std::function 后释放锁，再执行用户回调，避免回调反过来操作控制器时死锁。
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_frameCallback;
    }

    if (!callback) {
        return;
    }

    const uint64_t width = imageData->GetWidth();
    const uint64_t height = imageData->GetHeight();

    size_t byteCount = 0;
    if (!calculateRgb24Size(width, height, byteCount)) {
        setLastError("Galaxy frame size is invalid.");
        return;
    }

    // ConvertToRGB24 返回的是 SDK 管理的临时数据地址，不能把指针直接交给显示层长期使用。
    void* rgbBuffer = imageData->ConvertToRGB24(
        GX_BIT_0_7,
        GX_RAW2RGB_NEIGHBOUR,
        true
    );

    if (rgbBuffer == nullptr) {
        setLastError("Convert Galaxy frame to RGB24 failed.");
        return;
    }

    GalaxyCameraFrame frame;
    frame.width = static_cast<int>(width);
    frame.height = static_cast<int>(height);
    frame.frameId = imageData->GetFrameID();
    frame.rgb24.resize(byteCount);

    // 复制成自己持有的内存，确保回调函数返回后上层仍然可以安全使用这帧。
    std::memcpy(frame.rgb24.data(), rgbBuffer, byteCount);

    callback(std::move(frame));
}

void GalaxyCameraController::setLastError(std::string message)
{
    const std::string text = std::move(message);
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError = text;
    }

    std::cerr << text << std::endl;
}

} // namespace learnopengl::ui
