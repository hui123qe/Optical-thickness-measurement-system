#include "log_page.h"

#include "widget_factory.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
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
    connect(exportButton_, &QPushButton::clicked, this, &LogPage::exportRequested);
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
