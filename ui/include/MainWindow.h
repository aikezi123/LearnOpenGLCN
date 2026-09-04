#pragma once

#include <QMainWindow>

class QTreeWidgetItem;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace engineeringlab::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // 将装配层创建好的功能页面注册到指定导航分类。
    void addBusinessPage(
        const QString& categoryName,
        const QString& pageName,
        QWidget* page
    );

private:
    void initUIStyle();
    void initPages();
    void connectSignals();

    QTreeWidgetItem* addCategoryNode(const QString& name);
    QTreeWidgetItem* findCategoryNode(const QString& name) const;
    void addPageToCategory(
        QTreeWidgetItem* parent,
        const QString& name,
        QWidget* page
    );
    void addRootBusinessPage(const QString& name, QWidget* page);
    QWidget* createHomePage();
    void decorateChildNodeUI(QTreeWidgetItem* item);

    Ui::MainWindow* m_ui;
};

} // namespace engineeringlab::ui
