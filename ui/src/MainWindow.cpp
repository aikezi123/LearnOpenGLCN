#include "MainWindow.h"
#include "ui/ui_MainWindow.h"

#include <QBrush>
#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace engineeringlab::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    setWindowTitle(QStringLiteral("EngineeringLab"));
    resize(1280, 800);

    initUIStyle();
    connectSignals();
    initPages();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::initUIStyle()
{
    m_ui->centralwidget->setStyleSheet(QStringLiteral("background-color: #f3f6f8;"));
    m_ui->contentStack->setStyleSheet(QStringLiteral("QStackedWidget { background-color: #f3f6f8; }"));

    m_ui->navigationTree->setRootIsDecorated(false);
    m_ui->navigationTree->setFocusPolicy(Qt::NoFocus);
    m_ui->navigationTree->setUniformRowHeights(true);
    m_ui->navigationTree->setIndentation(24);
    m_ui->navigationTree->setHeaderHidden(true);
    m_ui->navigationTree->setMinimumWidth(250);
    m_ui->navigationTree->setMaximumWidth(300);
    m_ui->navigationTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_ui->navigationTree->setStyleSheet(QStringLiteral(R"(
        QTreeWidget {
            background-color: #fbfcfe;
            border: none;
            border-right: 1px solid #d7dee8;
            outline: none;
            padding-top: 12px;
            padding-bottom: 12px;
        }

        QTreeWidget::branch {
            image: none;
            border-image: none;
        }

        QTreeWidget::item {
            min-height: 38px;
            margin: 2px 10px;
            padding-left: 12px;
            color: #29323d;
            border: 1px solid transparent;
            border-radius: 6px;
        }

        QTreeWidget::item:selected {
            background-color: #e8f4f8;
            border: 1px solid #b8dbe5;
            color: #006d7c;
        }

        QTreeWidget::item:hover {
            background-color: #eef3f7;
        }
    )"));
}

void MainWindow::connectSignals()
{
    connect(
        m_ui->navigationTree,
        &QTreeWidget::currentItemChanged,
        this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
            if (current == nullptr) {
                return;
            }

            const QVariant pageIndex = current->data(0, Qt::UserRole);
            if (pageIndex.isValid()) {
                m_ui->contentStack->setCurrentIndex(pageIndex.toInt());
            }
        }
    );

    connect(
        m_ui->navigationTree,
        &QTreeWidget::itemClicked,
        this,
        [](QTreeWidgetItem* item, int) {
            if (item != nullptr && item->childCount() > 0) {
                item->setExpanded(!item->isExpanded());
            }
        }
    );
}

void MainWindow::initPages()
{
    addRootBusinessPage(QStringLiteral("首页"), createHomePage());

    m_ui->navigationTree->expandAll();

    if (m_ui->navigationTree->topLevelItemCount() > 0) {
        m_ui->navigationTree->setCurrentItem(m_ui->navigationTree->topLevelItem(0));
    }
}

QTreeWidgetItem* MainWindow::addCategoryNode(const QString& name)
{
    auto* item = new QTreeWidgetItem(m_ui->navigationTree, QStringList() << name);

    QFont font = item->font(0);
    font.setBold(true);
    font.setPointSize(10);
    item->setFont(0, font);
    item->setForeground(0, QBrush(QColor(QStringLiteral("#4c5968"))));
    item->setSizeHint(0, QSize(-1, 42));

    return item;
}

QTreeWidgetItem* MainWindow::findCategoryNode(const QString& name) const
{
    const int itemCount = m_ui->navigationTree->topLevelItemCount();
    for (int index = 0; index < itemCount; ++index) {
        QTreeWidgetItem* item = m_ui->navigationTree->topLevelItem(index);
        if (item != nullptr
            && !item->data(0, Qt::UserRole).isValid()
            && item->text(0) == name) {
            return item;
        }
    }

    return nullptr;
}

void MainWindow::addBusinessPage(
    const QString& categoryName,
    const QString& pageName,
    QWidget* page
)
{
    QTreeWidgetItem* category = findCategoryNode(categoryName);
    if (category == nullptr) {
        category = addCategoryNode(categoryName);
    }

    addPageToCategory(category, pageName, page);
    category->setExpanded(true);
}

void MainWindow::addPageToCategory(
    QTreeWidgetItem* parent,
    const QString& name,
    QWidget* page
)
{
    if (parent == nullptr || page == nullptr) {
        return;
    }

    const int index = m_ui->contentStack->addWidget(page);
    auto* item = new QTreeWidgetItem(parent, QStringList() << name);
    item->setData(0, Qt::UserRole, index);
    decorateChildNodeUI(item);
}

void MainWindow::addRootBusinessPage(const QString& name, QWidget* page)
{
    if (page == nullptr) {
        return;
    }

    const int index = m_ui->contentStack->addWidget(page);
    auto* item = new QTreeWidgetItem(m_ui->navigationTree, QStringList() << name);
    item->setData(0, Qt::UserRole, index);

    QFont font = item->font(0);
    font.setBold(true);
    font.setPointSize(10);
    item->setFont(0, font);
    item->setForeground(0, QBrush(QColor(QStringLiteral("#26313d"))));
    item->setSizeHint(0, QSize(-1, 42));
}

QWidget* MainWindow::createHomePage()
{
    auto* page = new QWidget(m_ui->contentStack);
    page->setObjectName(QStringLiteral("homePage"));
    page->setStyleSheet(QStringLiteral(R"(
        QWidget#homePage {
            background-color: #f3f6f8;
        }

        QFrame#homePanel {
            background-color: #ffffff;
            border: 1px solid #d8e1ea;
            border-radius: 6px;
        }

        QLabel#homeEyebrow {
            color: #007a8a;
            font-size: 12px;
            font-weight: 600;
        }

        QLabel#homeTitle {
            color: #1f2933;
            font-size: 26px;
            font-weight: 700;
        }

        QLabel#homeSubtitle {
            color: #5c6670;
            font-size: 13px;
        }

        QLabel#panelTitle {
            color: #26313d;
            font-size: 15px;
            font-weight: 700;
        }

        QLabel#panelText {
            color: #65717d;
            font-size: 12px;
        }
    )"));

    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(18);

    auto* eyebrow = new QLabel(QStringLiteral("Qt / OpenGL Workspace"), page);
    eyebrow->setObjectName(QStringLiteral("homeEyebrow"));

    auto* title = new QLabel(QStringLiteral("EngineeringLab"), page);
    title->setObjectName(QStringLiteral("homeTitle"));

    auto* subtitle = new QLabel(QStringLiteral("工业相机图像采集与 OpenGL 显示实验台"), page);
    subtitle->setObjectName(QStringLiteral("homeSubtitle"));

    auto* panelRow = new QHBoxLayout();
    panelRow->setSpacing(14);

    auto* cameraPanel = new QFrame(page);
    cameraPanel->setObjectName(QStringLiteral("homePanel"));
    auto* cameraLayout = new QVBoxLayout(cameraPanel);
    cameraLayout->setContentsMargins(18, 16, 18, 16);
    cameraLayout->setSpacing(8);

    auto* cameraTitle = new QLabel(QStringLiteral("相机模块"), cameraPanel);
    cameraTitle->setObjectName(QStringLiteral("panelTitle"));

    auto* cameraText = new QLabel(QStringLiteral("相机链路正在按 Domain 到 UI 的顺序重新重构。"), cameraPanel);
    cameraText->setObjectName(QStringLiteral("panelText"));
    cameraText->setWordWrap(true);

    cameraLayout->addWidget(cameraTitle);
    cameraLayout->addWidget(cameraText);
    cameraLayout->addStretch();

    auto* renderPanel = new QFrame(page);
    renderPanel->setObjectName(QStringLiteral("homePanel"));
    auto* renderLayout = new QVBoxLayout(renderPanel);
    renderLayout->setContentsMargins(18, 16, 18, 16);
    renderLayout->setSpacing(8);

    auto* renderTitle = new QLabel(QStringLiteral("OpenGL 显示"), renderPanel);
    renderTitle->setObjectName(QStringLiteral("panelTitle"));

    auto* renderText = new QLabel(QStringLiteral("当前图像上传仍由 QOpenGLWidget 管理，下一阶段可继续拆分渲染资源。"), renderPanel);
    renderText->setObjectName(QStringLiteral("panelText"));
    renderText->setWordWrap(true);

    renderLayout->addWidget(renderTitle);
    renderLayout->addWidget(renderText);
    renderLayout->addStretch();

    panelRow->addWidget(cameraPanel);
    panelRow->addWidget(renderPanel);

    layout->addWidget(eyebrow);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(12);
    layout->addLayout(panelRow);
    layout->addStretch();

    return page;
}

void MainWindow::decorateChildNodeUI(QTreeWidgetItem* item)
{
    if (item == nullptr) {
        return;
    }

    QFont font = item->font(0);
    font.setPointSize(10);
    item->setFont(0, font);
    item->setForeground(0, QBrush(QColor(QStringLiteral("#3b4652"))));
    item->setSizeHint(0, QSize(-1, 36));
}

} // namespace engineeringlab::ui
