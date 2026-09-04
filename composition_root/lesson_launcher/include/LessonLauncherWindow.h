#pragma once

#include <QMainWindow>
#include <QString>

class QByteArray;
class QLabel;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace engineeringlab::app {

class LessonLauncherWindow final : public QMainWindow {
public:
    explicit LessonLauncherWindow(QWidget* parent = nullptr);
    ~LessonLauncherWindow() override;

private:
    void setupUi();
    void applyStyle();
    void connectSignals();
    void populateLessons();
    void decorateChapterItem(QTreeWidgetItem* item);
    void decorateLessonItem(QTreeWidgetItem* item);
    void setCurrentLessonById(const QString& id);
    void selectLesson(QTreeWidgetItem* item);
    void startSelectedLesson();
    void stopRunningLesson();
    void updateProcessButtons();
    void appendProcessOutput(const QByteArray& output);
    void appendLine(const QString& text);

    QTreeWidget* m_navigationTree = nullptr;
    QLabel* m_chapterLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_descriptionLabel = nullptr;
    QLabel* m_lessonIdLabel = nullptr;
    QPushButton* m_runButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QPlainTextEdit* m_outputText = nullptr;
    QProcess* m_process = nullptr;
    QString m_currentLessonId;
};

} // namespace engineeringlab::app
