#include "log_page.h"

#include "widget_factory.h"

#include <xlsxdocument.h>
#include <xlsxformat.h>

#include <QAbstractItemView>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>

LogPage::LogPage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    layout->addWidget(WidgetFactory::createSectionTitle(
        QStringLiteral("日志记录"),
        QStringLiteral("从数据库查询自动任务终态日志，可按完成时间和关键字动态筛选")));

    QHBoxLayout* actions = new QHBoxLayout;
    QLabel* hint = new QLabel(QStringLiteral("只有任务结果为“完成”的记录可以导出数据"));
    hint->setProperty("role", "muted");
    queryStatus_ = new QLabel;
    queryStatus_->setProperty("role", "muted");
    exportButton_ = WidgetFactory::createPrimaryButton(QStringLiteral("导出选中记录"));
    exportButton_->setEnabled(false);
    actions->addWidget(hint);
    actions->addWidget(queryStatus_);
    actions->addStretch();
    actions->addWidget(exportButton_);
    layout->addLayout(actions);

    QHBoxLayout* filters = new QHBoxLayout;
    startTime_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    startTime_->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    startTime_->setCalendarPopup(true);
    endTime_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1));
    endTime_->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    endTime_->setCalendarPopup(true);
    keyword_ = new QLineEdit;
    keyword_->setPlaceholderText(QStringLiteral("搜索任务类型、结果或说明"));
    QPushButton* queryButton = new QPushButton(QStringLiteral("查询"));
    filters->addWidget(new QLabel(QStringLiteral("开始时间")));
    filters->addWidget(startTime_);
    filters->addWidget(new QLabel(QStringLiteral("结束时间")));
    filters->addWidget(endTime_);
    filters->addWidget(keyword_, 1);
    filters->addWidget(queryButton);
    layout->addLayout(filters);

    logTable_ = new QTableWidget(0, 4);
    logTable_->setHorizontalHeaderLabels({
        QStringLiteral("任务类型"),
        QStringLiteral("完成时间"),
        QStringLiteral("任务结果"),
        QStringLiteral("说明")});
    logTable_->verticalHeader()->setVisible(false);
    logTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    logTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    logTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    logTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    logTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(logTable_, 1);

    filterTimer_ = new QTimer(this);
    filterTimer_->setSingleShot(true);
    filterTimer_->setInterval(200);
    const auto scheduleRefresh = [this] {
        filterTimer_->start();
    };
    connect(startTime_, &QDateTimeEdit::dateTimeChanged, this, scheduleRefresh);
    connect(endTime_, &QDateTimeEdit::dateTimeChanged, this, scheduleRefresh);
    connect(keyword_, &QLineEdit::textChanged, this, scheduleRefresh);
    connect(filterTimer_, &QTimer::timeout, this, &LogPage::refresh);
    connect(queryButton, &QPushButton::clicked, this, &LogPage::refresh);
    connect(keyword_, &QLineEdit::returnPressed, this, &LogPage::refresh);
    connect(logTable_, &QTableWidget::itemSelectionChanged, this, [this] {
        const int row = logTable_->currentRow();
        const bool completed = row >= 0
            && logTable_->item(row, 2) != nullptr
            && logTable_->item(row, 2)->text() == QStringLiteral("完成");
        exportButton_->setEnabled(completed);
    });
    connect(exportButton_, &QPushButton::clicked, this, &LogPage::exportSelectedRecord);
    QTimer::singleShot(0, this, &LogPage::refresh);
}

void LogPage::refresh()
{
    filterTimer_->stop();
    logTable_->clearSelection();
    logTable_->setRowCount(0);
    exportButton_->setEnabled(false);

    if (startTime_->dateTime() > endTime_->dateTime()) {
        queryStatus_->setText(QStringLiteral("开始时间不能晚于结束时间"));
        return;
    }

    QString errorMessage;
    const QList<TaskLogRecord> records = database_.queryTaskLogs(
        startTime_->dateTime(),
        endTime_->dateTime(),
        keyword_->text(),
        &errorMessage);
    if (!errorMessage.isEmpty()) {
        queryStatus_->setText(errorMessage);
        return;
    }

    logTable_->setRowCount(records.size());
    for (int row = 0; row < records.size(); ++row) {
        const TaskLogRecord& record = records.at(row);
        QTableWidgetItem* taskType = new QTableWidgetItem(record.taskType);
        taskType->setData(Qt::UserRole, record.id);
        taskType->setData(Qt::UserRole + 1, record.runId);
        logTable_->setItem(row, 0, taskType);
        logTable_->setItem(
            row,
            1,
            new QTableWidgetItem(record.finishedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
        logTable_->setItem(row, 2, new QTableWidgetItem(record.result));
        logTable_->setItem(row, 3, new QTableWidgetItem(record.detail));
    }
    queryStatus_->setText(QStringLiteral("共 %1 条").arg(records.size()));
}

void LogPage::exportSelectedRecord()
{
    const int row = logTable_->currentRow();
    QTableWidgetItem* taskTypeItem = row >= 0 ? logTable_->item(row, 0) : nullptr;
    QTableWidgetItem* resultItem = row >= 0 ? logTable_->item(row, 2) : nullptr;
    if (taskTypeItem == nullptr || resultItem == nullptr
        || resultItem->text() != QStringLiteral("完成")) {
        QMessageBox::warning(
            this,
            QStringLiteral("无法导出记录"),
            QStringLiteral("请选择一条结果为“完成”的任务记录。"));
        return;
    }

    const QString runId = taskTypeItem->data(Qt::UserRole + 1).toString();
    QString errorMessage;
    const QList<MeasurementRecord> measurements =
        database_.queryMeasurements(runId, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("导出失败"), errorMessage);
        return;
    }
    if (measurements.isEmpty()) {
        QMessageBox::warning(
            this,
            QStringLiteral("无法导出记录"),
            QStringLiteral("选中任务没有可导出的测量数据。"));
        return;
    }

    const QDateTime exportedAt = QDateTime::currentDateTime();
    const QString fileName = QStringLiteral("测量记录_%1.xlsx")
                                 .arg(exportedAt.toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    QSettings settings(
        QSettings::IniFormat,
        QSettings::UserScope,
        QStringLiteral("OpticalThicknessMeasurementSystem"),
        QStringLiteral("OpticalThicknessUi"));
    const QString directorySettingKey =
        QStringLiteral("export/measurementRecordDirectory");
    QString defaultDirectory = settings.value(directorySettingKey).toString();
    if (defaultDirectory.isEmpty() || !QDir(defaultDirectory).exists()) {
        defaultDirectory =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    if (defaultDirectory.isEmpty() || !QDir(defaultDirectory).exists()) {
        defaultDirectory = QDir::homePath();
    }
    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出测量记录"),
        QDir(defaultDirectory).filePath(fileName),
        QStringLiteral("Excel 工作簿 (*.xlsx)"));
    if (filePath.isEmpty()) {
        return;
    }
    if (!filePath.endsWith(QStringLiteral(".xlsx"), Qt::CaseInsensitive)) {
        filePath.append(QStringLiteral(".xlsx"));
    }

    QXlsx::Document workbook;
    bool workbookReady = workbook.addSheet(QStringLiteral("测量数据"));

    QXlsx::Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);

    QXlsx::Format numberFormat;
    numberFormat.setNumberFormat(QStringLiteral("0.000"));

    QXlsx::Format dateTimeFormat;
    dateTimeFormat.setNumberFormat(QStringLiteral("yyyy-mm-dd hh:mm:ss.000"));

    const QStringList headers{
        QStringLiteral("点序号"),
        QStringLiteral("点位说明"),
        QStringLiteral("电机X(mm)"),
        QStringLiteral("电机Y(mm)"),
        QStringLiteral("物料xw(mm)"),
        QStringLiteral("物料yw(mm)"),
        QStringLiteral("厚度(um)"),
        QStringLiteral("测量时间")};
    for (int column = 0; column < headers.size(); ++column) {
        workbookReady = workbook.write(1, column + 1, headers.at(column), headerFormat)
            && workbookReady;
    }

    int worksheetRow = 2;
    for (const MeasurementRecord& measurement : measurements) {
        workbookReady = workbook.write(worksheetRow, 1, measurement.pointIndex)
            && workbookReady;
        workbookReady = workbook.write(
                            worksheetRow, 2, measurement.pointDescription)
            && workbookReady;
        workbookReady = workbook.write(
                            worksheetRow, 3, measurement.motorPosition.x(), numberFormat)
            && workbookReady;
        workbookReady = workbook.write(
                            worksheetRow, 4, measurement.motorPosition.y(), numberFormat)
            && workbookReady;
        workbookReady = workbook.write(
                            worksheetRow, 5, measurement.workpiecePosition.x(), numberFormat)
            && workbookReady;
        workbookReady = workbook.write(
                            worksheetRow, 6, measurement.workpiecePosition.y(), numberFormat)
            && workbookReady;
        workbookReady = workbook.write(
                            worksheetRow, 7, measurement.thicknessMicrometers, numberFormat)
            && workbookReady;
        workbookReady = workbook.write(
                            worksheetRow,
                            8,
                            QVariant::fromValue(measurement.measuredAt),
                            dateTimeFormat)
            && workbookReady;
        ++worksheetRow;
    }

    workbookReady = workbook.setRowHeight(1, 22.0) && workbookReady;
    workbookReady = workbook.setColumnWidth(1, 10.0) && workbookReady;
    workbookReady = workbook.setColumnWidth(2, 20.0) && workbookReady;
    workbookReady = workbook.setColumnWidth(3, 7, 15.0) && workbookReady;
    workbookReady = workbook.setColumnWidth(8, 24.0) && workbookReady;
    if (!workbookReady) {
        QMessageBox::critical(
            this,
            QStringLiteral("导出失败"),
            QStringLiteral("无法生成 Excel 工作簿内容。"));
        return;
    }

    if (!workbook.saveAs(filePath)) {
        QMessageBox::critical(
            this,
            QStringLiteral("导出失败"),
            QStringLiteral("无法写入 Excel 文件，请检查保存目录和文件占用状态。"));
        return;
    }

    settings.setValue(directorySettingKey, QFileInfo(filePath).absolutePath());
    settings.sync();
    const bool directoryRemembered = settings.status() == QSettings::NoError;
    QMessageBox::information(
        this,
        QStringLiteral("导出完成"),
        directoryRemembered
            ? QStringLiteral("测量记录已导出至：\n%1")
                  .arg(QDir::toNativeSeparators(filePath))
            : QStringLiteral("测量记录已导出至：\n%1\n\n但无法记录本次保存目录。")
                  .arg(QDir::toNativeSeparators(filePath)));
}
