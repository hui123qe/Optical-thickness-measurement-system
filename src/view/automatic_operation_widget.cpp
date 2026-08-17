#include "automatic_operation_widget.h"

#include "../workflow/task_point_queue.h"
#include "widget_factory.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

AutomaticOperationWidget::AutomaticOperationWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 18, 16, 16);
    layout->setSpacing(18);

    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(18);

    QGroupBox* taskGroup = new QGroupBox(QStringLiteral("任务配置"));
    QVBoxLayout* taskGroupLayout = new QVBoxLayout(taskGroup);

    QHBoxLayout* configurationManagementLayout = new QHBoxLayout;
    loadButton_ = new QPushButton(QStringLiteral("加载配置"));
    saveButton_ = new QPushButton(QStringLiteral("保存配置"));
    configurationManagementLayout->addStretch();
    configurationManagementLayout->addWidget(loadButton_);
    configurationManagementLayout->addWidget(saveButton_);
    taskGroupLayout->addLayout(configurationManagementLayout);

    QFormLayout* modeLayout = new QFormLayout;
    taskMode_ = new QComboBox;
    taskMode_->addItems({QStringLiteral("矩形阵列"), QStringLiteral("圆形阵列"), QStringLiteral("加载 CSV 文件")});
    modeLayout->addRow(QStringLiteral("任务模式"), taskMode_);
    taskGroupLayout->addLayout(modeLayout);

    modeConfiguration_ = new QStackedWidget;

    QWidget* rectanglePage = new QWidget;
    QFormLayout* rectangleLayout = new QFormLayout(rectanglePage);
    QLabel* rectangleDirectionHint = new QLabel(QStringLiteral(
        "四个角点输入顺序不限。列方向沿矩形较长边，行方向沿较短边；"
        "正方形时，列方向沿更接近物料 X 轴的边。点队列按行蛇形生成。"));
    rectangleDirectionHint->setWordWrap(true);
    rectangleDirectionHint->setProperty("role", "muted");
    rectangleLayout->addRow(rectangleDirectionHint);
    for (std::size_t index = 0; index < rectangleCornerXInputs_.size(); ++index) {
        rectangleCornerXInputs_[index] = WidgetFactory::createCoordinateInput();
        rectangleCornerYInputs_[index] = WidgetFactory::createCoordinateInput();
        QWidget* cornerInputs = new QWidget;
        QHBoxLayout* cornerLayout = new QHBoxLayout(cornerInputs);
        cornerLayout->setContentsMargins(0, 0, 0, 0);
        cornerLayout->addWidget(new QLabel(QStringLiteral("X")));
        cornerLayout->addWidget(rectangleCornerXInputs_[index]);
        cornerLayout->addWidget(new QLabel(QStringLiteral("Y")));
        cornerLayout->addWidget(rectangleCornerYInputs_[index]);
        rectangleLayout->addRow(
            QStringLiteral("角点 %1 / mm").arg(static_cast<int>(index + 1)),
            cornerInputs);
    }
    rectangleRowCount_ = new QSpinBox;
    rectangleRowCount_->setRange(1, 10000);
    rectangleColumnCount_ = new QSpinBox;
    rectangleColumnCount_->setRange(1, 10000);
    rectangleLayout->addRow(QStringLiteral("行数"), rectangleRowCount_);
    rectangleLayout->addRow(QStringLiteral("列数"), rectangleColumnCount_);
    modeConfiguration_->addWidget(rectanglePage);

    QWidget* circlePage = new QWidget;
    QFormLayout* circleLayout = new QFormLayout(circlePage);
    QLabel* circleHint = new QLabel(QStringLiteral(
        "圆心首先入队；圆环在最大半径内按圆环数量等距分布，并从内向外生成。"
        "未覆盖圆环使用默认点数，角度间隔为 360° ÷ 点数；每圈从物料 X 正方向逆时针生成。"));
    circleHint->setWordWrap(true);
    circleHint->setProperty("role", "muted");
    circleLayout->addRow(circleHint);
    circleCenterX_ = WidgetFactory::createCoordinateInput();
    circleCenterY_ = WidgetFactory::createCoordinateInput();
    circleMaximumRadius_ = WidgetFactory::createCoordinateInput();
    circleMaximumRadius_->setRange(0.001, 100000.0);
    circleRingCount_ = new QSpinBox;
    circleRingCount_->setRange(1, 10000);
    circleDefaultPointCount_ = new QSpinBox;
    circleDefaultPointCount_->setRange(3, 10000);
    circleLayout->addRow(QStringLiteral("中心 X / mm"), circleCenterX_);
    circleLayout->addRow(QStringLiteral("中心 Y / mm"), circleCenterY_);
    circleLayout->addRow(QStringLiteral("最大半径 / mm"), circleMaximumRadius_);
    circleLayout->addRow(QStringLiteral("圆环数量"), circleRingCount_);
    circleLayout->addRow(QStringLiteral("默认每圈点数"), circleDefaultPointCount_);

    QWidget* circleOverrideEditor = new QWidget;
    QHBoxLayout* circleOverrideEditorLayout = new QHBoxLayout(circleOverrideEditor);
    circleOverrideEditorLayout->setContentsMargins(0, 0, 0, 0);
    circleOverrideRingIndex_ = new QSpinBox;
    circleOverrideRingIndex_->setRange(1, 10000);
    circleOverridePointCount_ = new QSpinBox;
    circleOverridePointCount_->setRange(3, 10000);
    setCircleOverrideButton_ = new QPushButton(QStringLiteral("设置/更新覆盖"));
    removeCircleOverrideButton_ = new QPushButton(QStringLiteral("删除选中覆盖"));
    circleOverrideEditorLayout->addWidget(new QLabel(QStringLiteral("圆环")));
    circleOverrideEditorLayout->addWidget(circleOverrideRingIndex_);
    circleOverrideEditorLayout->addWidget(new QLabel(QStringLiteral("点数")));
    circleOverrideEditorLayout->addWidget(circleOverridePointCount_);
    circleOverrideEditorLayout->addWidget(setCircleOverrideButton_);
    circleOverrideEditorLayout->addWidget(removeCircleOverrideButton_);
    circleOverrideEditorLayout->addStretch();
    circleLayout->addRow(QStringLiteral("单圈覆盖设置"), circleOverrideEditor);

    circleRingOverrides_ = new QTableWidget;
    circleRingOverrides_->setObjectName(QStringLiteral("circleRingOverrides"));
    circleRingOverrides_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    circleRingOverrides_->setColumnCount(2);
    circleRingOverrides_->setHorizontalHeaderLabels(
        {QStringLiteral("圆环序号"), QStringLiteral("覆盖点数")});
    circleRingOverrides_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    circleRingOverrides_->verticalHeader()->setVisible(false);
    circleRingOverrides_->setSelectionBehavior(QAbstractItemView::SelectRows);
    circleRingOverrides_->setSelectionMode(QAbstractItemView::SingleSelection);
    circleRingOverrides_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    circleRingOverrides_->setAlternatingRowColors(true);
    circleRingOverrides_->setMinimumHeight(300);
    circleLayout->addRow(QStringLiteral("已设置覆盖"), circleRingOverrides_);
    modeConfiguration_->addWidget(circlePage);

    QWidget* csvPage = new QWidget;
    QFormLayout* csvLayout = new QFormLayout(csvPage);
    csvPath_ = new QLineEdit;
    csvPath_->setPlaceholderText(QStringLiteral("选择坐标 CSV 文件"));
    csvPath_->setReadOnly(true);
    browseButton_ = new QPushButton(QStringLiteral("浏览…"));
    QWidget* fileRow = new QWidget;
    QHBoxLayout* fileLayout = new QHBoxLayout(fileRow);
    fileLayout->setContentsMargins(0, 0, 0, 0);
    fileLayout->addWidget(csvPath_, 1);
    fileLayout->addWidget(browseButton_);
    csvLayout->addRow(QStringLiteral("坐标文件"), fileRow);
    modeConfiguration_->addWidget(csvPage);
    taskGroupLayout->addWidget(modeConfiguration_);

    QHBoxLayout* configurationActionLayout = new QHBoxLayout;
    resetButton_ = new QPushButton(QStringLiteral("重置配置"));
    confirmButton_ = WidgetFactory::createPrimaryButton(QStringLiteral("确认配置"));
    configurationActionLayout->addStretch();
    configurationActionLayout->addWidget(resetButton_);
    configurationActionLayout->addWidget(confirmButton_);
    taskGroupLayout->addLayout(configurationActionLayout);
    contentLayout->addWidget(taskGroup, 4);

    QGroupBox* actionGroup = new QGroupBox(QStringLiteral("动作"));
    QVBoxLayout* actionLayout = new QVBoxLayout(actionGroup);
    QPushButton* homeButton = new QPushButton(QStringLiteral("回零"));
    startButton_ = WidgetFactory::createPrimaryButton(QStringLiteral("测试开始"));
    QPushButton* terminateButton = WidgetFactory::createDangerButton(QStringLiteral("终止"));
    actionLayout->addWidget(homeButton);
    actionLayout->addWidget(startButton_);
    actionLayout->addWidget(terminateButton);
    actionLayout->addStretch();
    contentLayout->addWidget(actionGroup, 1);

    layout->addLayout(contentLayout, 1);

    connect(taskMode_, &QComboBox::currentIndexChanged, modeConfiguration_, &QStackedWidget::setCurrentIndex);
    connect(setCircleOverrideButton_, &QPushButton::clicked, this, &AutomaticOperationWidget::setCircleRingOverride);
    connect(removeCircleOverrideButton_, &QPushButton::clicked, this, &AutomaticOperationWidget::removeSelectedCircleRingOverride);
    connect(circleRingOverrides_, &QTableWidget::itemSelectionChanged, this, &AutomaticOperationWidget::loadSelectedCircleRingOverride);
    connect(circleRingCount_, &QSpinBox::valueChanged, this, &AutomaticOperationWidget::updateCircleOverrideRingRange);
    connect(browseButton_, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择坐标文件"), QString(), QStringLiteral("CSV 文件 (*.csv)"));
        if (!path.isEmpty()) {
            csvPath_->setText(path);
        }
    });
    connect(loadButton_, &QPushButton::clicked, this, &AutomaticOperationWidget::loadConfiguration);
    connect(saveButton_, &QPushButton::clicked, this, &AutomaticOperationWidget::saveConfiguration);
    connect(resetButton_, &QPushButton::clicked, this, &AutomaticOperationWidget::resetConfiguration);
    connect(confirmButton_, &QPushButton::clicked, this, &AutomaticOperationWidget::confirmConfiguration);
    connect(startButton_, &QPushButton::clicked, this, &AutomaticOperationWidget::startTask);
    connect(terminateButton, &QPushButton::clicked, this, [this] {
        emit taskTerminationRequested();
    });

    applyConfiguration(TaskConfiguration{});
    setConfigurationLocked(false);
}

void AutomaticOperationWidget::setCircleRingOverride()
{
    upsertCircleRingOverride(
        circleOverrideRingIndex_->value(),
        circleOverridePointCount_->value());
}

void AutomaticOperationWidget::removeSelectedCircleRingOverride()
{
    const int selectedRow = circleRingOverrides_->currentRow();
    if (selectedRow >= 0) {
        circleRingOverrides_->removeRow(selectedRow);
    }
}

void AutomaticOperationWidget::loadSelectedCircleRingOverride()
{
    const int selectedRow = circleRingOverrides_->currentRow();
    if (selectedRow < 0) {
        return;
    }

    circleOverrideRingIndex_->setValue(
        circleRingOverrides_->item(selectedRow, 0)->data(Qt::UserRole).toInt());
    circleOverridePointCount_->setValue(
        circleRingOverrides_->item(selectedRow, 1)->data(Qt::UserRole).toInt());
}

void AutomaticOperationWidget::updateCircleOverrideRingRange(int ringCount)
{
    circleOverrideRingIndex_->setMaximum(ringCount);
    for (int row = circleRingOverrides_->rowCount() - 1; row >= 0; --row) {
        if (circleRingOverrides_->item(row, 0)->data(Qt::UserRole).toInt() > ringCount) {
            circleRingOverrides_->removeRow(row);
        }
    }
}

int AutomaticOperationWidget::findCircleRingOverrideRow(int ringIndex) const
{
    for (int row = 0; row < circleRingOverrides_->rowCount(); ++row) {
        if (circleRingOverrides_->item(row, 0)->data(Qt::UserRole).toInt() == ringIndex) {
            return row;
        }
    }
    return -1;
}

void AutomaticOperationWidget::upsertCircleRingOverride(int ringIndex, int pointCount)
{
    int row = findCircleRingOverrideRow(ringIndex);
    if (row < 0) {
        row = 0;
        while (row < circleRingOverrides_->rowCount()
               && circleRingOverrides_->item(row, 0)->data(Qt::UserRole).toInt() < ringIndex) {
            ++row;
        }
        circleRingOverrides_->insertRow(row);
        circleRingOverrides_->setItem(row, 0, new QTableWidgetItem);
        circleRingOverrides_->setItem(row, 1, new QTableWidgetItem);
    }

    QTableWidgetItem* ringItem = circleRingOverrides_->item(row, 0);
    ringItem->setText(QString::number(ringIndex));
    ringItem->setData(Qt::UserRole, ringIndex);
    ringItem->setTextAlignment(Qt::AlignCenter);

    QTableWidgetItem* pointCountItem = circleRingOverrides_->item(row, 1);
    pointCountItem->setText(QString::number(pointCount));
    pointCountItem->setData(Qt::UserRole, pointCount);
    pointCountItem->setTextAlignment(Qt::AlignCenter);

    circleRingOverrides_->selectRow(row);
    circleRingOverrides_->scrollToItem(ringItem);
}

TaskConfiguration AutomaticOperationWidget::configurationFromEditor() const
{
    TaskConfiguration configuration;
    configuration.mode = static_cast<TaskMode>(taskMode_->currentIndex());
    for (std::size_t index = 0; index < configuration.rectangle.corners.size(); ++index) {
        configuration.rectangle.corners[index] = QPointF(
            rectangleCornerXInputs_[index]->value(),
            rectangleCornerYInputs_[index]->value());
    }
    configuration.rectangle.rowCount = rectangleRowCount_->value();
    configuration.rectangle.columnCount = rectangleColumnCount_->value();
    configuration.circle.centerX = circleCenterX_->value();
    configuration.circle.centerY = circleCenterY_->value();
    configuration.circle.maximumRadius = circleMaximumRadius_->value();
    configuration.circle.ringCount = circleRingCount_->value();
    configuration.circle.defaultPointCount = circleDefaultPointCount_->value();
    configuration.circle.ringPointOverrides.clear();
    configuration.circle.ringPointOverrides.reserve(circleRingOverrides_->rowCount());
    for (int row = 0; row < circleRingOverrides_->rowCount(); ++row) {
        configuration.circle.ringPointOverrides.append(CircleRingPointOverride{
            circleRingOverrides_->item(row, 0)->data(Qt::UserRole).toInt(),
            circleRingOverrides_->item(row, 1)->data(Qt::UserRole).toInt()
        });
    }
    configuration.csv.filePath = csvPath_->text();
    return configuration;
}

void AutomaticOperationWidget::applyConfiguration(const TaskConfiguration& configuration)
{
    taskMode_->setCurrentIndex(static_cast<int>(configuration.mode));
    for (std::size_t index = 0; index < configuration.rectangle.corners.size(); ++index) {
        rectangleCornerXInputs_[index]->setValue(configuration.rectangle.corners[index].x());
        rectangleCornerYInputs_[index]->setValue(configuration.rectangle.corners[index].y());
    }
    rectangleRowCount_->setValue(configuration.rectangle.rowCount);
    rectangleColumnCount_->setValue(configuration.rectangle.columnCount);
    circleCenterX_->setValue(configuration.circle.centerX);
    circleCenterY_->setValue(configuration.circle.centerY);
    circleMaximumRadius_->setValue(configuration.circle.maximumRadius);
    circleRingCount_->setValue(configuration.circle.ringCount);
    circleDefaultPointCount_->setValue(configuration.circle.defaultPointCount);
    circleOverrideRingIndex_->setValue(1);
    circleOverridePointCount_->setValue(configuration.circle.defaultPointCount);
    circleRingOverrides_->setRowCount(0);
    for (const CircleRingPointOverride& pointOverride : configuration.circle.ringPointOverrides) {
        upsertCircleRingOverride(pointOverride.ringIndex, pointOverride.pointCount);
    }
    csvPath_->setText(configuration.csv.filePath);
}

void AutomaticOperationWidget::setConfigurationLocked(bool locked)
{
    configurationLocked_ = locked;
    taskMode_->setEnabled(!locked);
    for (QDoubleSpinBox* input : rectangleCornerXInputs_) {
        input->setReadOnly(locked);
    }
    for (QDoubleSpinBox* input : rectangleCornerYInputs_) {
        input->setReadOnly(locked);
    }
    rectangleRowCount_->setReadOnly(locked);
    rectangleColumnCount_->setReadOnly(locked);
    circleCenterX_->setReadOnly(locked);
    circleCenterY_->setReadOnly(locked);
    circleMaximumRadius_->setReadOnly(locked);
    circleRingCount_->setReadOnly(locked);
    circleDefaultPointCount_->setReadOnly(locked);
    circleOverrideRingIndex_->setReadOnly(locked);
    circleOverridePointCount_->setReadOnly(locked);
    setCircleOverrideButton_->setEnabled(!locked);
    removeCircleOverrideButton_->setEnabled(!locked);
    browseButton_->setEnabled(!locked);
    loadButton_->setEnabled(!locked);
    confirmButton_->setEnabled(!locked);
    startButton_->setEnabled(locked);
}

void AutomaticOperationWidget::loadConfiguration()
{
    QString errorMessage;
    const QStringList names = configurationStore_.configurationNames(&errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("加载配置失败"), errorMessage);
        return;
    }
    if (names.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("加载配置"), QStringLiteral("暂无已保存配置。"));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("加载配置"));
    QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout* selectionLayout = new QFormLayout;
    QComboBox* savedConfiguration = new QComboBox;
    savedConfiguration->addItems(names);
    selectionLayout->addRow(QStringLiteral("选择配置"), savedConfiguration);
    dialogLayout->addLayout(selectionLayout);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("加载"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialogLayout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const std::optional<TaskConfiguration> configuration = configurationStore_.load(savedConfiguration->currentText(), &errorMessage);
    if (!configuration.has_value()) {
        QMessageBox::critical(this, QStringLiteral("加载配置失败"), errorMessage);
        return;
    }
    applyConfiguration(*configuration);
    setConfigurationLocked(false);
    emit taskStateChangeRequested(QStringLiteral("待确认"), QStringLiteral("已加载配置“%1”，请检查后确认。")
        .arg(savedConfiguration->currentText()), QStringLiteral("waiting"));
}

void AutomaticOperationWidget::saveConfiguration()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("保存配置"));
    QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);
    QFormLayout* nameLayout = new QFormLayout;
    QLineEdit* configurationName = new QLineEdit;
    configurationName->setMaxLength(64);
    configurationName->setPlaceholderText(QStringLiteral("输入配置名称"));
    nameLayout->addRow(QStringLiteral("配置名称"), configurationName);
    dialogLayout->addLayout(nameLayout);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialogLayout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString name = configurationName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法保存配置"), QStringLiteral("配置名称不能为空。"));
        return;
    }

    QString errorMessage;
    const QStringList names = configurationStore_.configurationNames(&errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, QStringLiteral("保存配置失败"), errorMessage);
        return;
    }
    if (names.contains(name) && QMessageBox::question(
            this, QStringLiteral("覆盖配置"), QStringLiteral("配置“%1”已存在，是否覆盖？").arg(name))
            != QMessageBox::Yes) {
        return;
    }

    if (!configurationStore_.save(name, configurationFromEditor(), &errorMessage)) {
        QMessageBox::critical(this, QStringLiteral("保存配置失败"), errorMessage);
        return;
    }
    QMessageBox::information(this, QStringLiteral("保存配置"), QStringLiteral("配置“%1”已保存。").arg(name));
}

void AutomaticOperationWidget::resetConfiguration()
{
    if (configurationLocked_ && QMessageBox::question(
            this,
            QStringLiteral("重置配置"),
            QStringLiteral("重置将解除当前确认状态并恢复默认参数，是否继续？")) != QMessageBox::Yes) {
        return;
    }
    applyConfiguration(TaskConfiguration{});
    setConfigurationLocked(false);
    emit taskStateChangeRequested(QStringLiteral("待配置"), QStringLiteral("任务配置已重置，请重新配置并确认。"), QStringLiteral("waiting"));
}

void AutomaticOperationWidget::confirmConfiguration()
{
    const TaskConfiguration configuration = configurationFromEditor();
    const QString errorMessage = configuration.validationError();
    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("配置校验失败"), errorMessage);
        return;
    }
    if (QMessageBox::question(this, QStringLiteral("确认配置"), configuration.summary() + QStringLiteral("\n\n确认后配置将锁定。"))
        != QMessageBox::Yes) {
        return;
    }

    confirmedConfiguration_ = configuration;
    setConfigurationLocked(true);
    emit taskStateChangeRequested(QStringLiteral("配置已确认"), QStringLiteral("任务配置已锁定，可以开始测试。"), QStringLiteral("ready"));
}

void AutomaticOperationWidget::startTask()
{
    if (!configurationLocked_) {
        QMessageBox::warning(this, QStringLiteral("无法开始"), QStringLiteral("请先确认任务配置。"));
        return;
    }
    QString errorMessage;
    const std::optional<QList<WorkpieceTaskPoint>> pointQueue =
        createWorkpiecePointQueue(confirmedConfiguration_, &errorMessage);
    if (!pointQueue.has_value()) {
        QMessageBox::critical(this, QStringLiteral("无法生成点队列"), errorMessage);
        emit taskStateChangeRequested(QStringLiteral("队列生成失败"), errorMessage, QStringLiteral("terminated"));
        return;
    }

    emit workpiecePointQueuePrepared(confirmedConfiguration_.modeName(), *pointQueue);
    QMessageBox::information(
        this,
        QStringLiteral("任务摘要确认"),
        confirmedConfiguration_.summary()
            + QStringLiteral("\n\n已生成 %1 个物料坐标点并提交自动测量任务。")
                  .arg(pointQueue->size()));
}
