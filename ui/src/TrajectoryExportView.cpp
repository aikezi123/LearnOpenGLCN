#include "TrajectoryExportView.h"

#include <trajectory/ArchimedeanSpiral2DGenerator.h>

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace engineeringlab::ui {
namespace {

using engineeringlab::domain::trajectory::ArchimedeanSpiral2DGenerator;
using ExportCompletion = std::pair<bool, QString>;

QDoubleSpinBox* createDistanceSpinBox(QWidget* parent,
                                      double minimum,
                                      double maximum,
                                      double value,
                                      int decimals,
                                      double step)
{
    auto* spinBox = new QDoubleSpinBox(parent);
    spinBox->setRange(minimum, maximum);
    spinBox->setDecimals(decimals);
    spinBox->setSingleStep(step);
    spinBox->setValue(value);
    spinBox->setSuffix(QStringLiteral(" mm"));
    spinBox->setAlignment(Qt::AlignRight);
    return spinBox;
}

QDoubleSpinBox* createPlainDoubleSpinBox(QWidget* parent,
                                         double minimum,
                                         double maximum,
                                         double value,
                                         int decimals,
                                         double step)
{
    auto* spinBox = new QDoubleSpinBox(parent);
    spinBox->setRange(minimum, maximum);
    spinBox->setDecimals(decimals);
    spinBox->setSingleStep(step);
    spinBox->setValue(value);
    spinBox->setAlignment(Qt::AlignRight);
    return spinBox;
}

QDoubleSpinBox* createRangeCellSpinBox(QWidget* parent, double value)
{
    auto* spinBox = createDistanceSpinBox(parent, 0.0, 100.0, value, 6, 0.001);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    return spinBox;
}

std::filesystem::path toFileSystemPath(const QString& path)
{
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toStdString());
#endif
}

QDoubleSpinBox* rangeCellSpinBox(QTableWidget* table, int row, int column)
{
    return qobject_cast<QDoubleSpinBox*>(table->cellWidget(row, column));
}

std::string rangeFilePrefix(std::size_t rangeIndex)
{
    return "archimedean_spiral_range_" + std::to_string(rangeIndex + 1);
}

class TrajectoryTextFiles final {
public:
    TrajectoryTextFiles(const std::filesystem::path& xyPath,
                        const std::filesystem::path& iterationsPath)
        : m_xyFile(xyPath)
        , m_iterationsFile(iterationsPath)
    {
        if (!m_xyFile || !m_iterationsFile) {
            throw std::runtime_error("failed to open trajectory output file");
        }

        m_xyFile << std::setprecision(17);
        m_iterationsFile << std::setprecision(17);
    }

    void write(std::size_t pointNumber, const ArchimedeanSpiral2DGenerator::Point& point)
    {
        m_xyFile << point.xMm << ' ' << point.yMm << '\n';
        m_iterationsFile << pointNumber << ' '
                         << point.iterationsFromPreviousPoint << '\n';
    }

    void flushAndValidate()
    {
        m_xyFile.flush();
        m_iterationsFile.flush();
        if (!m_xyFile || !m_iterationsFile) {
            throw std::runtime_error("failed while writing trajectory output file");
        }
    }

private:
    std::ofstream m_xyFile;
    std::ofstream m_iterationsFile;
};

ExportCompletion runTrajectoryExport(
    ArchimedeanSpiral2DGenerator::GenerationParameters parameters,
    std::vector<ArchimedeanSpiral2DGenerator::RadiusRange> ranges,
    QString outputDirectoryText,
    const std::shared_ptr<std::atomic_bool>& cancelExport)
{
    try {
        using Clock = std::chrono::steady_clock;
        using Milliseconds = std::chrono::duration<double, std::milli>;

        const auto exportStart = Clock::now();
        const std::filesystem::path outputDirectory = toFileSystemPath(outputDirectoryText);
        std::filesystem::create_directories(outputDirectory);

        const std::filesystem::path xyPath = outputDirectory / "archimedean_spiral_xy.txt";
        const std::filesystem::path iterationsPath =
            outputDirectory / "archimedean_spiral_iterations.txt";
        TrajectoryTextFiles totalFiles(xyPath, iterationsPath);

        std::size_t totalPointCount = 0;
        int maxIterations = 0;
        double generationMilliseconds = 0.0;
        double writingMilliseconds = 0.0;

        for (std::size_t rangeIndex = 0; rangeIndex < ranges.size(); ++rangeIndex) {
            if (cancelExport->load(std::memory_order_relaxed)) {
                return {false, QStringLiteral("导出已取消。")};
            }

            ArchimedeanSpiral2DGenerator generator(parameters);
            const auto generationStart = Clock::now();
            generator.generateInRadiusRange(ranges[rangeIndex]);
            const double rangeGenerationMilliseconds =
                Milliseconds(Clock::now() - generationStart).count();
            generationMilliseconds += rangeGenerationMilliseconds;

            std::clog << "[TrajectoryExport] range=" << (rangeIndex + 1)
                      << " points=" << generator.points().size()
                      << " generation_ms=" << std::fixed << std::setprecision(3)
                      << rangeGenerationMilliseconds << '\n';

            const auto writingStart = Clock::now();
            const std::string prefix = rangeFilePrefix(rangeIndex);
            TrajectoryTextFiles rangeFiles(
                outputDirectory / (prefix + "_xy.txt"),
                outputDirectory / (prefix + "_iterations.txt")
            );

            const auto& points = generator.points();
            for (std::size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
                if ((pointIndex % 1024 == 0)
                    && cancelExport->load(std::memory_order_relaxed)) {
                    return {false, QStringLiteral("导出已取消。")};
                }

                const auto& point = points[pointIndex];
                rangeFiles.write(pointIndex + 1, point);

                // 后续分段的第一个点与上一段终点重合，总文件中只保留一个。
                if (rangeIndex == 0 || pointIndex > 0) {
                    ++totalPointCount;
                    totalFiles.write(totalPointCount, point);
                }

                maxIterations = std::max(maxIterations,
                                         point.iterationsFromPreviousPoint);
            }

            rangeFiles.flushAndValidate();
            writingMilliseconds += Milliseconds(Clock::now() - writingStart).count();
        }

        const auto finalFlushStart = Clock::now();
        totalFiles.flushAndValidate();
        writingMilliseconds += Milliseconds(Clock::now() - finalFlushStart).count();

        const double exportMilliseconds =
            Milliseconds(Clock::now() - exportStart).count();

        std::clog << "[TrajectoryExport] total_points=" << totalPointCount
                  << " ranges=" << ranges.size()
                  << " generation_ms=" << generationMilliseconds
                  << " writing_ms=" << writingMilliseconds
                  << " export_ms=" << exportMilliseconds << '\n';

        return {
            true,
            QStringLiteral("已导出 %1 个点，共 %2 个分段，最大迭代次数 %3。\n"
                           "轨迹生成耗时 %4 ms，文件写入耗时 %5 ms，总耗时 %6 ms。\n"
                           "%7\n%8")
                .arg(static_cast<qulonglong>(totalPointCount))
                .arg(static_cast<qulonglong>(ranges.size()))
                .arg(maxIterations)
                .arg(generationMilliseconds, 0, 'f', 3)
                .arg(writingMilliseconds, 0, 'f', 3)
                .arg(exportMilliseconds, 0, 'f', 3)
                .arg(QString::fromStdWString(xyPath.wstring()))
                .arg(QString::fromStdWString(iterationsPath.wstring()))
        };
    } catch (const std::exception& error) {
        return {
            false,
            QStringLiteral("导出失败：%1").arg(QString::fromLocal8Bit(error.what()))
        };
    }
}

} // namespace

TrajectoryExportView::TrajectoryExportView(QWidget* parent)
    : QWidget(parent)
    , m_exportWatcher(new QFutureWatcher<ExportCompletion>(this))
    , m_cancelExport(std::make_shared<std::atomic_bool>(false))
{
    buildUi();
    connectSignals();
}

TrajectoryExportView::~TrajectoryExportView()
{
    if (m_exportWatcher != nullptr && m_exportWatcher->isRunning()) {
        m_cancelExport->store(true, std::memory_order_relaxed);
        disconnect(m_exportWatcher, nullptr, this, nullptr);
        m_exportWatcher->waitForFinished();
    }
}

void TrajectoryExportView::buildUi()
{
    setObjectName(QStringLiteral("trajectoryExportView"));
    setStyleSheet(QStringLiteral(R"(
        QWidget#trajectoryExportView {
            background-color: #f3f6f8;
        }

        QGroupBox {
            background-color: #ffffff;
            border: 1px solid #d8e1ea;
            border-radius: 6px;
            color: #26313d;
            font-weight: 700;
            margin-top: 12px;
            padding: 14px 12px 12px 12px;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 4px;
        }

        QLabel {
            color: #3b4652;
        }

        QLineEdit, QDoubleSpinBox, QSpinBox {
            min-height: 28px;
            border: 1px solid #c9d4df;
            border-radius: 4px;
            padding: 2px 6px;
            background-color: #ffffff;
            color: #1f2933;
        }

        QTableWidget {
            background-color: #ffffff;
            border: 1px solid #c9d4df;
            gridline-color: #dbe3eb;
            color: #1f2933;
            selection-background-color: #e8f4f8;
            selection-color: #1f2933;
        }

        QHeaderView::section {
            background-color: #eef3f7;
            border: none;
            border-right: 1px solid #d8e1ea;
            border-bottom: 1px solid #d8e1ea;
            color: #3b4652;
            font-weight: 600;
            padding: 6px;
        }

        QPushButton {
            min-height: 30px;
            border: 1px solid #9fc9d2;
            border-radius: 4px;
            background-color: #e8f4f8;
            color: #006d7c;
            padding: 4px 12px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: #d8edf3;
        }

        QPushButton#exportButton {
            background-color: #007a8a;
            border-color: #007a8a;
            color: #ffffff;
        }

        QPushButton#exportButton:hover {
            background-color: #006b79;
        }
    )"));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(14);

    auto* title = new QLabel(QStringLiteral("二维阿基米德螺旋线导出"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto* parameterGroup = new QGroupBox(QStringLiteral("生成参数"), this);
    auto* parameterLayout = new QFormLayout(parameterGroup);
    parameterLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    parameterLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    parameterLayout->setHorizontalSpacing(18);
    parameterLayout->setVerticalSpacing(10);

    m_startRadiusSpin = createDistanceSpinBox(parameterGroup, 0.0, 100.0, 0.0, 6, 0.001);
    m_trackSpacingSpin = createDistanceSpinBox(parameterGroup, 0.000001, 10.0, 0.004, 6, 0.001);
    m_distanceToleranceSpin = createPlainDoubleSpinBox(parameterGroup, 1e-12, 1.0, 1e-6, 12, 1e-6);

    m_maxIterationsSpin = new QSpinBox(parameterGroup);
    m_maxIterationsSpin->setRange(1, 1000);
    m_maxIterationsSpin->setValue(30);
    m_maxIterationsSpin->setAlignment(Qt::AlignRight);

    m_maxBracketExpansionsSpin = new QSpinBox(parameterGroup);
    m_maxBracketExpansionsSpin->setRange(0, 1000);
    m_maxBracketExpansionsSpin->setValue(32);
    m_maxBracketExpansionsSpin->setAlignment(Qt::AlignRight);

    parameterLayout->addRow(QStringLiteral("起始半径"), m_startRadiusSpin);
    parameterLayout->addRow(QStringLiteral("线间距"), m_trackSpacingSpin);
    parameterLayout->addRow(QStringLiteral("误差容限"), m_distanceToleranceSpin);
    parameterLayout->addRow(QStringLiteral("最大迭代次数"), m_maxIterationsSpin);
    parameterLayout->addRow(QStringLiteral("右边界扩展次数"), m_maxBracketExpansionsSpin);

    auto* rangeGroup = new QGroupBox(QStringLiteral("半径分段"), this);
    auto* rangeLayout = new QVBoxLayout(rangeGroup);
    rangeLayout->setSpacing(10);

    m_radiusRangeTable = new QTableWidget(rangeGroup);
    m_radiusRangeTable->setColumnCount(3);
    m_radiusRangeTable->setHorizontalHeaderLabels(
        QStringList() << QStringLiteral("起始半径")
                      << QStringLiteral("结束半径")
                      << QStringLiteral("点间距")
    );
    m_radiusRangeTable->horizontalHeader()->setStretchLastSection(true);
    m_radiusRangeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_radiusRangeTable->verticalHeader()->setVisible(false);
    m_radiusRangeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_radiusRangeTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_radiusRangeTable->setAlternatingRowColors(true);
    m_radiusRangeTable->setMinimumHeight(150);

    auto* rangeButtonLayout = new QHBoxLayout();
    rangeButtonLayout->addStretch();
    m_addRangeButton = new QPushButton(QStringLiteral("添加分段"), rangeGroup);
    m_removeRangeButton = new QPushButton(QStringLiteral("删除选中分段"), rangeGroup);
    rangeButtonLayout->addWidget(m_addRangeButton);
    rangeButtonLayout->addWidget(m_removeRangeButton);

    rangeLayout->addWidget(m_radiusRangeTable);
    rangeLayout->addLayout(rangeButtonLayout);

    addRadiusRangeRow(0.0, 0.3, 0.005);
    addRadiusRangeRow(0.3, 1.0, 0.010);
    addRadiusRangeRow(1.0, 3.5, 0.050);

    auto* outputGroup = new QGroupBox(QStringLiteral("导出"), this);
    auto* outputLayout = new QGridLayout(outputGroup);
    outputLayout->setHorizontalSpacing(10);
    outputLayout->setVerticalSpacing(10);

    const QString defaultOutputDirectory = QDir::cleanPath(
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../../../generated/spiral_txt_export"))
    );

    m_outputDirectoryEdit = new QLineEdit(defaultOutputDirectory, outputGroup);
    m_browseButton = new QPushButton(QStringLiteral("浏览"), outputGroup);
    m_exportButton = new QPushButton(QStringLiteral("导出 txt"), outputGroup);
    m_exportButton->setObjectName(QStringLiteral("exportButton"));

    m_statusLabel = new QLabel(QStringLiteral("未导出"), outputGroup);
    m_statusLabel->setWordWrap(true);

    outputLayout->addWidget(new QLabel(QStringLiteral("输出目录"), outputGroup), 0, 0);
    outputLayout->addWidget(m_outputDirectoryEdit, 0, 1);
    outputLayout->addWidget(m_browseButton, 0, 2);
    outputLayout->addWidget(m_exportButton, 1, 2);
    outputLayout->addWidget(m_statusLabel, 1, 0, 1, 2);
    outputLayout->setColumnStretch(1, 1);

    mainLayout->addWidget(title);
    mainLayout->addWidget(parameterGroup);
    mainLayout->addWidget(rangeGroup);
    mainLayout->addWidget(outputGroup);
    mainLayout->addStretch();
}

void TrajectoryExportView::connectSignals()
{
    connect(m_browseButton, &QPushButton::clicked, this, [this]() {
        browseOutputDirectory();
    });

    connect(m_addRangeButton, &QPushButton::clicked, this, [this]() {
        addDefaultRadiusRange();
    });

    connect(m_removeRangeButton, &QPushButton::clicked, this, [this]() {
        removeSelectedRadiusRange();
    });

    connect(m_exportButton, &QPushButton::clicked, this, [this]() {
        exportTrajectory();
    });

    connect(m_exportWatcher, &QFutureWatcher<ExportCompletion>::finished, this, [this]() {
        handleExportFinished();
    });
}

void TrajectoryExportView::browseOutputDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择导出目录"),
        m_outputDirectoryEdit->text()
    );

    if (!directory.isEmpty()) {
        m_outputDirectoryEdit->setText(directory);
    }
}

void TrajectoryExportView::addRadiusRangeRow(double startRadiusMm,
                                             double endRadiusMm,
                                             double pointSpacingMm)
{
    const int row = m_radiusRangeTable->rowCount();
    m_radiusRangeTable->insertRow(row);
    m_radiusRangeTable->setCellWidget(m_radiusRangeTable->rowCount() - 1, 0,
                                      createRangeCellSpinBox(m_radiusRangeTable, startRadiusMm));
    m_radiusRangeTable->setCellWidget(m_radiusRangeTable->rowCount() - 1, 1,
                                      createRangeCellSpinBox(m_radiusRangeTable, endRadiusMm));
    m_radiusRangeTable->setCellWidget(m_radiusRangeTable->rowCount() - 1, 2,
                                      createRangeCellSpinBox(m_radiusRangeTable, pointSpacingMm));
    m_radiusRangeTable->selectRow(row);
}

void TrajectoryExportView::addDefaultRadiusRange()
{
    double startRadiusMm = m_startRadiusSpin->value();
    double pointSpacingMm = 0.003;

    if (m_radiusRangeTable->rowCount() > 0) {
        if (auto* previousEnd = rangeCellSpinBox(m_radiusRangeTable, m_radiusRangeTable->rowCount() - 1, 1)) {
            startRadiusMm = previousEnd->value();
        }

        if (auto* previousSpacing = rangeCellSpinBox(m_radiusRangeTable, m_radiusRangeTable->rowCount() - 1, 2)) {
            pointSpacingMm = previousSpacing->value();
        }
    }

    addRadiusRangeRow(startRadiusMm, startRadiusMm + 0.1, pointSpacingMm);
}

void TrajectoryExportView::removeSelectedRadiusRange()
{
    if (m_radiusRangeTable->rowCount() <= 1) {
        setStatusText(QStringLiteral("至少需要保留一个半径分段。"), true);
        return;
    }

    int row = m_radiusRangeTable->currentRow();
    if (row < 0) {
        row = m_radiusRangeTable->rowCount() - 1;
    }

    m_radiusRangeTable->removeRow(row);
    m_radiusRangeTable->selectRow(std::min(row, m_radiusRangeTable->rowCount() - 1));
}

void TrajectoryExportView::exportTrajectory()
{
    if (m_exportWatcher->isRunning()) {
        setStatusText(QStringLiteral("正在导出，请稍候。"), false);
        return;
    }

    try {
        const QString outputDirectoryText = m_outputDirectoryEdit->text().trimmed();
        if (outputDirectoryText.isEmpty()) {
            setStatusText(QStringLiteral("输出目录不能为空。"), true);
            return;
        }

        ArchimedeanSpiral2DGenerator::GenerationParameters parameters;
        parameters.startRadiusMm = m_startRadiusSpin->value();
        parameters.trackSpacingMm = m_trackSpacingSpin->value();
        parameters.distanceToleranceMm = m_distanceToleranceSpin->value();
        parameters.maxIterations = m_maxIterationsSpin->value();
        parameters.maxBracketExpansions = m_maxBracketExpansionsSpin->value();
        parameters.deltaThetaMethod = ArchimedeanSpiral2DGenerator::DeltaThetaMethod::Bisection;
        parameters.appendRangeEndPoint = true;

        std::vector<ArchimedeanSpiral2DGenerator::RadiusRange> ranges;
        ranges.reserve(static_cast<std::size_t>(m_radiusRangeTable->rowCount()));

        for (int row = 0; row < m_radiusRangeTable->rowCount(); ++row) {
            auto* startSpin = rangeCellSpinBox(m_radiusRangeTable, row, 0);
            auto* endSpin = rangeCellSpinBox(m_radiusRangeTable, row, 1);
            auto* spacingSpin = rangeCellSpinBox(m_radiusRangeTable, row, 2);
            if (startSpin == nullptr || endSpin == nullptr || spacingSpin == nullptr) {
                setStatusText(QStringLiteral("半径分段表存在无效单元格。"), true);
                return;
            }

            ArchimedeanSpiral2DGenerator::RadiusRange range;
            range.startRadiusMm = startSpin->value();
            range.endRadiusMm = endSpin->value();
            range.pointSpacingMm = spacingSpin->value();

            if (range.endRadiusMm <= range.startRadiusMm) {
                setStatusText(QStringLiteral("第 %1 段的结束半径必须大于起始半径。").arg(row + 1), true);
                return;
            }

            if (!ranges.empty()) {
                const double previousEnd = ranges.back().endRadiusMm;
                if (std::abs(range.startRadiusMm - previousEnd) > parameters.distanceToleranceMm) {
                    setStatusText(
                        QStringLiteral("第 %1 段必须从上一段结束半径 %2 mm 开始。")
                            .arg(row + 1)
                            .arg(previousEnd, 0, 'g', 12),
                        true
                    );
                    return;
                }
            }

            ranges.push_back(range);
        }

        if (ranges.empty()) {
            setStatusText(QStringLiteral("至少需要一个半径分段。"), true);
            return;
        }

        m_cancelExport->store(false, std::memory_order_relaxed);
        setExportControlsEnabled(false);
        setStatusText(QStringLiteral("正在后台生成轨迹并写入 txt 文件..."), false);

        const auto cancelExport = m_cancelExport;
        m_exportWatcher->setFuture(QtConcurrent::run(
            [parameters,
             ranges = std::move(ranges),
             outputDirectoryText,
             cancelExport]() mutable {
                return runTrajectoryExport(parameters,
                                           std::move(ranges),
                                           outputDirectoryText,
                                           cancelExport);
            }
        ));
    } catch (const std::exception& error) {
        setExportControlsEnabled(true);
        setStatusText(QStringLiteral("导出失败：%1").arg(QString::fromLocal8Bit(error.what())), true);
    }
}

void TrajectoryExportView::handleExportFinished()
{
    const ExportCompletion completion = m_exportWatcher->result();
    setExportControlsEnabled(true);
    setStatusText(completion.second, !completion.first);
}

void TrajectoryExportView::setExportControlsEnabled(bool enabled)
{
    m_startRadiusSpin->setEnabled(enabled);
    m_trackSpacingSpin->setEnabled(enabled);
    m_distanceToleranceSpin->setEnabled(enabled);
    m_maxIterationsSpin->setEnabled(enabled);
    m_maxBracketExpansionsSpin->setEnabled(enabled);
    m_radiusRangeTable->setEnabled(enabled);
    m_addRangeButton->setEnabled(enabled);
    m_removeRangeButton->setEnabled(enabled);
    m_outputDirectoryEdit->setEnabled(enabled);
    m_browseButton->setEnabled(enabled);
    m_exportButton->setEnabled(enabled);
}

void TrajectoryExportView::setStatusText(const QString& text, bool isError)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError
                                     ? QStringLiteral("color: #b42318;")
                                     : QStringLiteral("color: #28745d;"));
}

} // namespace engineeringlab::ui
