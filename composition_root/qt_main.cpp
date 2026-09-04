#include "AppComposition.h"

#include <QApplication>
#include <QMainWindow>
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

    engineeringlab::composition::AppComposition composition;
    std::unique_ptr<QMainWindow> window = composition.createMainWindow();
    window->show();

    return app.exec();
}
