#include "MainWindow.h"

#include "ui/ui_MainWindow.h"

namespace learnopengl::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

} // namespace learnopengl::ui
