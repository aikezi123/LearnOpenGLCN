#include "LessonLauncherWindow.h"

#include <lessons/LessonRegistry.h>

#include <QBrush>
#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSize>
#include <QSplitter>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <string_view>

namespace {

constexpr int kLessonIdRole = Qt::UserRole + 1;

QString toQString(std::string_view value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::string_view toStringView(const QByteArray& value)
{
    return std::string_view(value.constData(), static_cast<std::size_t>(value.size()));
}

const learnopengl::lessons::LessonEntry* findLesson(const QString& id)
{
    const QByteArray utf8Id = id.toUtf8();
    return learnopengl::lessons::findLesson(toStringView(utf8Id));
}

} // namespace

namespace learnopengl::app {

LessonLauncherWindow::LessonLauncherWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_process(new QProcess(this))
{
    setWindowTitle(QStringLiteral("LearnOpenGLCN Lessons"));
    resize(1180, 760);

    setupUi();
    connectSignals();
    populateLessons();
}

LessonLauncherWindow::~LessonLauncherWindow()
{
    stopRunningLesson();
}

void LessonLauncherWindow::setupUi()
{
    auto* splitter = new QSplitter(this);
    splitter->setObjectName(QStringLiteral("lessonSplitter"));
    splitter->setChildrenCollapsible(false);

    m_navigationTree = new QTreeWidget(splitter);
    m_navigationTree->setObjectName(QStringLiteral("lessonNavigationTree"));
    m_navigationTree->setHeaderHidden(true);
    m_navigationTree->setRootIsDecorated(false);
    m_navigationTree->setUniformRowHeights(true);
    m_navigationTree->setIndentation(18);
    m_navigationTree->setMinimumWidth(260);
    m_navigationTree->setMaximumWidth(340);
    m_navigationTree->header()->setStretchLastSection(true);

    auto* rightPanel = new QWidget(splitter);
    rightPanel->setObjectName(QStringLiteral("lessonRightPanel"));

    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(28, 24, 28, 24);
    rightLayout->setSpacing(14);

    m_chapterLabel = new QLabel(rightPanel);
    m_chapterLabel->setObjectName(QStringLiteral("lessonChapterLabel"));

    m_titleLabel = new QLabel(rightPanel);
    m_titleLabel->setObjectName(QStringLiteral("lessonTitleLabel"));

    m_descriptionLabel = new QLabel(rightPanel);
    m_descriptionLabel->setObjectName(QStringLiteral("lessonDescriptionLabel"));
    m_descriptionLabel->setWordWrap(true);

    auto* commandFrame = new QFrame(rightPanel);
    commandFrame->setObjectName(QStringLiteral("lessonCommandFrame"));
    auto* commandLayout = new QHBoxLayout(commandFrame);
    commandLayout->setContentsMargins(16, 14, 16, 14);
    commandLayout->setSpacing(10);

    m_lessonIdLabel = new QLabel(commandFrame);
    m_lessonIdLabel->setObjectName(QStringLiteral("lessonIdLabel"));

    m_runButton = new QPushButton(QStringLiteral("运行课程"), commandFrame);
    m_stopButton = new QPushButton(QStringLiteral("停止"), commandFrame);
    m_stopButton->setEnabled(false);

    commandLayout->addWidget(m_lessonIdLabel);
    commandLayout->addStretch();
    commandLayout->addWidget(m_runButton);
    commandLayout->addWidget(m_stopButton);

    m_outputText = new QPlainTextEdit(rightPanel);
    m_outputText->setObjectName(QStringLiteral("lessonOutputText"));
    m_outputText->setReadOnly(true);
    m_outputText->setPlaceholderText(QStringLiteral("课程进程输出会显示在这里。GLFW 课程画面会在单独窗口中打开。"));

    rightLayout->addWidget(m_chapterLabel);
    rightLayout->addWidget(m_titleLabel);
    rightLayout->addWidget(m_descriptionLabel);
    rightLayout->addWidget(commandFrame);
    rightLayout->addWidget(m_outputText, 1);

    splitter->addWidget(m_navigationTree);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);
    applyStyle();
}

void LessonLauncherWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(R"(
        QSplitter#lessonSplitter {
            background-color: #f4f7fa;
        }

        QTreeWidget#lessonNavigationTree {
            background-color: #fbfcfe;
            border: none;
            border-right: 1px solid #d8e1ea;
            outline: none;
            padding-top: 12px;
            padding-bottom: 12px;
        }

        QTreeWidget#lessonNavigationTree::branch {
            image: none;
            border-image: none;
        }

        QTreeWidget#lessonNavigationTree::item {
            min-height: 38px;
            margin: 2px 10px;
            padding-left: 12px;
            color: #2d3742;
            border: 1px solid transparent;
            border-radius: 6px;
        }

        QTreeWidget#lessonNavigationTree::item:selected {
            background-color: #e8f4f8;
            border: 1px solid #b9dae4;
            color: #006d7c;
        }

        QTreeWidget#lessonNavigationTree::item:hover {
            background-color: #eef3f7;
        }

        QWidget#lessonRightPanel {
            background-color: #f4f7fa;
        }

        QLabel#lessonChapterLabel {
            color: #007a8a;
            font-size: 12px;
            font-weight: 600;
        }

        QLabel#lessonTitleLabel {
            color: #1f2933;
            font-size: 25px;
            font-weight: 700;
        }

        QLabel#lessonDescriptionLabel {
            color: #596674;
            font-size: 13px;
        }

        QFrame#lessonCommandFrame {
            background-color: #ffffff;
            border: 1px solid #d8e1ea;
            border-radius: 6px;
        }

        QLabel#lessonIdLabel {
            color: #435160;
            font-family: Consolas, "Microsoft YaHei UI";
            font-size: 12px;
        }

        QPushButton {
            min-width: 82px;
            min-height: 30px;
            padding: 4px 12px;
            border: 1px solid #bdcad5;
            border-radius: 5px;
            background-color: #ffffff;
            color: #26313d;
        }

        QPushButton:hover {
            background-color: #eef6f8;
            border-color: #8fc7d3;
        }

        QPushButton:disabled {
            color: #98a4af;
            background-color: #edf1f5;
            border-color: #d0d8df;
        }

        QPlainTextEdit#lessonOutputText {
            background-color: #12181f;
            color: #d7e1ea;
            border: 1px solid #25313d;
            border-radius: 6px;
            padding: 10px;
            font-family: Consolas, "Microsoft YaHei UI";
            font-size: 12px;
        }
    )"));
}

void LessonLauncherWindow::connectSignals()
{
    connect(
        m_navigationTree,
        &QTreeWidget::currentItemChanged,
        this,
        [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
            selectLesson(current);
        }
    );

    connect(
        m_navigationTree,
        &QTreeWidget::itemClicked,
        this,
        [](QTreeWidgetItem* item, int) {
            if (item != nullptr && item->childCount() > 0) {
                item->setExpanded(!item->isExpanded());
            }
        }
    );

    connect(
        m_navigationTree,
        &QTreeWidget::itemDoubleClicked,
        this,
        [this](QTreeWidgetItem* item, int) {
            selectLesson(item);
            startSelectedLesson();
        }
    );

    connect(m_runButton, &QPushButton::clicked, this, [this]() {
        startSelectedLesson();
    });

    connect(m_stopButton, &QPushButton::clicked, this, [this]() {
        stopRunningLesson();
    });

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        appendProcessOutput(m_process->readAllStandardOutput());
    });

    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        appendProcessOutput(m_process->readAllStandardError());
    });

    connect(
        m_process,
        &QProcess::errorOccurred,
        this,
        [this](QProcess::ProcessError error) {
            appendLine(QStringLiteral("Process error: %1").arg(static_cast<int>(error)));
            updateProcessButtons();
        }
    );

    connect(
        m_process,
        &QProcess::finished,
        this,
        [this](int exitCode, QProcess::ExitStatus exitStatus) {
            appendLine(
                QStringLiteral("Lesson finished. exitCode=%1, exitStatus=%2")
                    .arg(exitCode)
                    .arg(exitStatus == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crash"))
            );
            updateProcessButtons();
        }
    );
}

void LessonLauncherWindow::populateLessons()
{
    QString currentChapter;
    QTreeWidgetItem* chapterItem = nullptr;

    for (const auto* lesson = learnopengl::lessons::lessonsBegin();
         lesson != learnopengl::lessons::lessonsEnd();
         ++lesson) {
        const QString chapter = toQString(lesson->chapter);
        if (chapter != currentChapter) {
            currentChapter = chapter;
            chapterItem = new QTreeWidgetItem(m_navigationTree, QStringList() << chapter);
            decorateChapterItem(chapterItem);
        }

        auto* lessonItem = new QTreeWidgetItem(chapterItem, QStringList() << toQString(lesson->title));
        lessonItem->setData(0, kLessonIdRole, toQString(lesson->id));
        lessonItem->setToolTip(0, toQString(lesson->description));
        decorateLessonItem(lessonItem);
    }

    m_navigationTree->expandAll();

    const learnopengl::lessons::LessonEntry* defaultLesson =
        learnopengl::lessons::findLesson(learnopengl::lessons::defaultLessonId());
    if (defaultLesson != nullptr) {
        setCurrentLessonById(toQString(defaultLesson->id));
    }
}

void LessonLauncherWindow::decorateChapterItem(QTreeWidgetItem* item)
{
    QFont font = item->font(0);
    font.setBold(true);
    font.setPointSize(10);
    item->setFont(0, font);
    item->setForeground(0, QBrush(QColor(QStringLiteral("#4c5968"))));
    item->setSizeHint(0, QSize(-1, 42));
}

void LessonLauncherWindow::decorateLessonItem(QTreeWidgetItem* item)
{
    QFont font = item->font(0);
    font.setPointSize(10);
    item->setFont(0, font);
    item->setForeground(0, QBrush(QColor(QStringLiteral("#3b4652"))));
    item->setSizeHint(0, QSize(-1, 36));
}

void LessonLauncherWindow::setCurrentLessonById(const QString& id)
{
    for (int i = 0; i < m_navigationTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* chapterItem = m_navigationTree->topLevelItem(i);
        for (int j = 0; j < chapterItem->childCount(); ++j) {
            QTreeWidgetItem* lessonItem = chapterItem->child(j);
            if (lessonItem->data(0, kLessonIdRole).toString() == id) {
                m_navigationTree->setCurrentItem(lessonItem);
                return;
            }
        }
    }
}

void LessonLauncherWindow::selectLesson(QTreeWidgetItem* item)
{
    if (item == nullptr) {
        return;
    }

    const QString id = item->data(0, kLessonIdRole).toString();
    if (id.isEmpty()) {
        return;
    }

    const learnopengl::lessons::LessonEntry* lesson = findLesson(id);
    if (lesson == nullptr) {
        return;
    }

    m_currentLessonId = id;
    m_chapterLabel->setText(toQString(lesson->chapter));
    m_titleLabel->setText(toQString(lesson->title));
    m_descriptionLabel->setText(toQString(lesson->description));
    m_lessonIdLabel->setText(QStringLiteral("Lesson ID: %1").arg(id));
    m_runButton->setEnabled(m_process->state() == QProcess::NotRunning);
}

void LessonLauncherWindow::startSelectedLesson()
{
    if (m_currentLessonId.isEmpty()) {
        appendLine(QStringLiteral("Please select a lesson first."));
        return;
    }

    if (m_process->state() != QProcess::NotRunning) {
        appendLine(QStringLiteral("A lesson is already running. Stop it before launching another one."));
        return;
    }

    m_outputText->clear();
    appendLine(QStringLiteral("Starting lesson: %1").arg(m_currentLessonId));

    m_process->setProgram(QCoreApplication::applicationFilePath());
    m_process->setArguments(QStringList() << m_currentLessonId);
    m_process->setWorkingDirectory(QCoreApplication::applicationDirPath());
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    m_process->start();

    updateProcessButtons();
}

void LessonLauncherWindow::stopRunningLesson()
{
    if (m_process->state() == QProcess::NotRunning) {
        updateProcessButtons();
        return;
    }

    appendLine(QStringLiteral("Stopping lesson process..."));
    m_process->terminate();
    if (!m_process->waitForFinished(1500)) {
        m_process->kill();
        m_process->waitForFinished(1500);
    }

    updateProcessButtons();
}

void LessonLauncherWindow::updateProcessButtons()
{
    const bool isRunning = m_process->state() != QProcess::NotRunning;
    m_runButton->setEnabled(!isRunning && !m_currentLessonId.isEmpty());
    m_stopButton->setEnabled(isRunning);
}

void LessonLauncherWindow::appendProcessOutput(const QByteArray& output)
{
    if (output.isEmpty()) {
        return;
    }

    appendLine(QString::fromLocal8Bit(output));
}

void LessonLauncherWindow::appendLine(const QString& text)
{
    m_outputText->appendPlainText(text.trimmed());
}

} // namespace learnopengl::app
