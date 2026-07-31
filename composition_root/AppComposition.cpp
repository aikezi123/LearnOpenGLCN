#include "AppComposition.h"

#include "modules/CameraComposition.h"
#include "modules/TrajectoryComposition.h"

#include <MainWindow.h>

#include <QMainWindow>
#include <QString>

#include <memory>

namespace learnopengl::composition {

std::unique_ptr<QMainWindow> AppComposition::createMainWindow() const
{
    auto mainWindow = std::make_unique<ui::MainWindow>();

    mainWindow->addBusinessPage(
        QStringLiteral("相机模块"),
        QStringLiteral("大恒相机预览"),
        CameraComposition::createPage(mainWindow.get())
    );

    mainWindow->addBusinessPage(
        QStringLiteral("轨迹算法"),
        QStringLiteral("螺旋线导出"),
        TrajectoryComposition::createPage(mainWindow.get())
    );

    return mainWindow;
}

} // namespace learnopengl::composition
