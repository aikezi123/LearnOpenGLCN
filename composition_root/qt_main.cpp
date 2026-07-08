#include "MainWindow.h"

#include <camera/CameraPreviewService.h>
#include <camera/ICameraDevice.h>
#include <camera/galaxy/GalaxyCameraController.h>

#include <QApplication>
#include <QSurfaceFormat>

#include <memory>

int main(int argc, char* argv[])
{
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    std::unique_ptr<learnopengl::application::ICameraDevice> cameraDevice =
        std::make_unique<learnopengl::infrastructure::camera::galaxy::GalaxyCameraController>();
    auto cameraPreview = std::make_unique<learnopengl::application::CameraPreviewService>(
        std::move(cameraDevice)
    );

    learnopengl::ui::MainWindow window(std::move(cameraPreview));
    window.show();

    return app.exec();
}
