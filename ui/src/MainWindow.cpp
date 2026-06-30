#include "MainWindow.h"
#include "CameraImageCaptureView.h"
#include "ui/ui_MainWindow.h"

namespace learnopengl::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    auto* cameraView = new CameraImageCaptureView(m_ui->centralwidget);
    m_ui->verticalLayout->replaceWidget(m_ui->widget, cameraView);
    delete m_ui->widget;
    m_ui->widget = nullptr;
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

} // namespace learnopengl::ui
