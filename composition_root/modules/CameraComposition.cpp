#include "CameraComposition.h"

#include <CameraImageCaptureView.h>
#include <camera/CameraCaptureService.h>
#include <camera/galaxy/GalaxyCameraController.h>

#include <memory>
#include <utility>

namespace engineeringlab::composition {

QWidget* CameraComposition::createPage(QWidget* parent)
{
    auto cameraDevice = std::make_unique<infrastructure::camera::galaxy::GalaxyCameraController>();

    auto cameraCaptureService =
        std::make_unique<application::CameraCaptureService>(
            std::move(cameraDevice)
        );

    return new ui::CameraImageCaptureView(
        std::move(cameraCaptureService),
        parent
    );
}

} // namespace engineeringlab::composition
