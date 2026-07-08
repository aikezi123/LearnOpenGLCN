#pragma once

#include <QMainWindow>

#include <memory>

class QTreeWidgetItem;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace learnopengl::application {
class CameraPreviewService;
}

namespace learnopengl::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(
        std::unique_ptr<application::CameraPreviewService> cameraPreview,
        QWidget* parent = nullptr
    );
    ~MainWindow() override;

private:
    void initUIStyle();
    void initPages(std::unique_ptr<application::CameraPreviewService> cameraPreview);
    void connectSignals();

    QTreeWidgetItem* addCategoryNode(const QString& name);
    void addBusinessPage(QTreeWidgetItem* parent, const QString& name, QWidget* page);
    void addRootBusinessPage(const QString& name, QWidget* page);
    QWidget* createHomePage();
    void decorateChildNodeUI(QTreeWidgetItem* item);

    Ui::MainWindow* m_ui;
};

} // namespace learnopengl::ui
