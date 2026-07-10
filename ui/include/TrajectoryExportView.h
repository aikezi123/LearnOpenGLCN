#pragma once

#include <QFutureWatcher>
#include <QString>
#include <QWidget>

#include <atomic>
#include <memory>
#include <utility>

class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QLabel;
class QTableWidget;

namespace learnopengl::ui {

class TrajectoryExportView final : public QWidget {
public:
    explicit TrajectoryExportView(QWidget* parent = nullptr);
    ~TrajectoryExportView() override;

private:
    void buildUi();
    void connectSignals();
    void browseOutputDirectory();
    void addRadiusRangeRow(double startRadiusMm, double endRadiusMm, double pointSpacingMm);
    void addDefaultRadiusRange();
    void removeSelectedRadiusRange();
    void exportTrajectory();
    void handleExportFinished();
    void setExportControlsEnabled(bool enabled);
    void setStatusText(const QString& text, bool isError);

    QDoubleSpinBox* m_startRadiusSpin = nullptr;
    QDoubleSpinBox* m_trackSpacingSpin = nullptr;
    QDoubleSpinBox* m_distanceToleranceSpin = nullptr;
    QSpinBox* m_maxIterationsSpin = nullptr;
    QSpinBox* m_maxBracketExpansionsSpin = nullptr;
    QTableWidget* m_radiusRangeTable = nullptr;
    QPushButton* m_addRangeButton = nullptr;
    QPushButton* m_removeRangeButton = nullptr;
    QLineEdit* m_outputDirectoryEdit = nullptr;
    QPushButton* m_browseButton = nullptr;
    QPushButton* m_exportButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QFutureWatcher<std::pair<bool, QString>>* m_exportWatcher = nullptr;
    std::shared_ptr<std::atomic_bool> m_cancelExport;
};

} // namespace learnopengl::ui
