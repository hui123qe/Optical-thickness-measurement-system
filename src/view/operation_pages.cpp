#include "operation_pages.h"

#include "../config/coordinate_transform_store.h"
#include "automatic_operation_widget.h"
#include "widget_factory.h"

#include <cmath>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>

#include <QtMath>

namespace {

QTransform createWorkpieceToMotorTransform(const CoordinateTransformConfiguration& configuration)
{
    const double radians = qDegreesToRadians(configuration.rotationDegrees);
    const double cosTheta = std::cos(radians);
    const double sinTheta = std::sin(radians);
    return QTransform(
        cosTheta,
        sinTheta,
        -sinTheta,
        cosTheta,
        configuration.translationX,
        configuration.translationY);
}

QString coordinateTransformDescription(const CoordinateTransformConfiguration& configuration)
{
    return QStringLiteral("θ = %1°，tx = %2 mm，ty = %3 mm")
        .arg(configuration.rotationDegrees, 0, 'f', 3)
        .arg(configuration.translationX, 0, 'f', 3)
        .arg(configuration.translationY, 0, 'f', 3);
}

QLabel* createColumnHeader(const QString& text)
{
    QLabel* header = new QLabel(text);
    header->setProperty("role", "formHeader");
    header->setFixedHeight(28);
    header->setAlignment(Qt::AlignCenter);
    return header;
}

} // namespace

MainPage::MainPage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    layout->addWidget(WidgetFactory::createSectionTitle(QStringLiteral("主界面"), QStringLiteral("可直接移动载物台，或按客户提供的物料测点进行引导")));
    layout->addWidget(createCoordinateTransformGroup());
    layout->addLayout(createLaserMeasurementControls());
    layout->addWidget(createOperationTabs(), 1);
}

QGroupBox* MainPage::createCoordinateTransformGroup()
{
    QGroupBox* transformGroup = new QGroupBox(QStringLiteral("物料坐标转换"));
    QHBoxLayout* transformLayout = new QHBoxLayout(transformGroup);
    transformSelector_ = new QComboBox;
    transformParameters_ = new QLabel;
    transformParameters_->setProperty("role", "muted");
    transformLayout->addWidget(new QLabel(QStringLiteral("转换方案")));
    transformLayout->addWidget(transformSelector_, 1);
    transformLayout->addWidget(transformParameters_, 2);

    CoordinateTransformStore configurationStore;
    QString transformError;
    const QList<CoordinateTransformConfiguration> configurations =
        configurationStore.configurations(&transformError);
    if (configurations.isEmpty()) {
        transformSelector_->addItem(QStringLiteral("无可用转换方案"));
        transformSelector_->setEnabled(false);
        transformParameters_->setText(transformError);
        return transformGroup;
    }

    coordinateTransforms_.reserve(configurations.size());
    coordinateTransformDescriptions_.reserve(configurations.size());
    for (const CoordinateTransformConfiguration& configuration : configurations) {
        transformSelector_->addItem(configuration.name);
        coordinateTransforms_.append(createWorkpieceToMotorTransform(configuration));
        coordinateTransformDescriptions_.append(coordinateTransformDescription(configuration));
    }

    connect(
        transformSelector_,
        &QComboBox::currentIndexChanged,
        this,
        &MainPage::applyCoordinateTransform);
    applyCoordinateTransform(transformSelector_->currentIndex());
    return transformGroup;
}

QHBoxLayout* MainPage::createLaserMeasurementControls()
{
    QHBoxLayout* layout = new QHBoxLayout;
    startLaserMeasurement_ =
        WidgetFactory::createPrimaryButton(QStringLiteral("开始测量"));
    stopLaserMeasurement_ = new QPushButton(QStringLiteral("停止测量"));
    laserMeasurementState_ =
        WidgetFactory::createStatusBadge(QStringLiteral("未测量"), QStringLiteral("offline"));
    layout->addWidget(new QLabel(QStringLiteral("激光传感器")));
    layout->addWidget(startLaserMeasurement_);
    layout->addWidget(stopLaserMeasurement_);
    layout->addSpacing(8);
    layout->addWidget(new QLabel(QStringLiteral("测量状态")));
    layout->addWidget(laserMeasurementState_);
    layout->addStretch();

    connect(startLaserMeasurement_, &QPushButton::clicked,
        this, &MainPage::laserMeasurementStartRequested);
    connect(stopLaserMeasurement_, &QPushButton::clicked,
        this, &MainPage::laserMeasurementStopRequested);
    updateLaserMeasurementControls();
    return layout;
}

void MainPage::setLaserConnectionState(bool connected)
{
    laserConnected_ = connected;
    if (!laserConnected_) {
        laserMeasuring_ = false;
    }
    updateLaserMeasurementControls();
}

void MainPage::setLaserMeasurementState(bool measuring)
{
    laserMeasuring_ = laserConnected_ && measuring;
    updateLaserMeasurementControls();
}

void MainPage::updateLaserMeasurementControls()
{
    if (startLaserMeasurement_ == nullptr
        || stopLaserMeasurement_ == nullptr
        || laserMeasurementState_ == nullptr) {
        return;
    }

    startLaserMeasurement_->setEnabled(laserConnected_ && !laserMeasuring_);
    stopLaserMeasurement_->setEnabled(laserConnected_ && laserMeasuring_);
    laserMeasurementState_->setText(
        laserMeasuring_ ? QStringLiteral("测量中") : QStringLiteral("未测量"));
    laserMeasurementState_->setProperty(
        "state",
        !laserConnected_
            ? QStringLiteral("offline")
            : laserMeasuring_ ? QStringLiteral("ready") : QStringLiteral("waiting"));
    laserMeasurementState_->style()->unpolish(laserMeasurementState_);
    laserMeasurementState_->style()->polish(laserMeasurementState_);
}

QTabWidget* MainPage::createOperationTabs()
{
    QTabWidget* tabs = new QTabWidget;
    ManualControlWidget* manualControl = new ManualControlWidget;
    AutomaticOperationWidget* automaticOperation = new AutomaticOperationWidget;
    tabs->addTab(manualControl, QStringLiteral("手动控制"));
    tabs->addTab(automaticOperation, QStringLiteral("自动化操作"));

    connect(manualControl, &ManualControlWidget::stageAxisAbsoluteMoveRequested, this, &MainPage::stageAxisAbsoluteMoveRequested);
    connect(manualControl, &ManualControlWidget::stageAbsoluteMoveRequested, this, &MainPage::stageAbsoluteMoveRequested);
    connect(manualControl, &ManualControlWidget::workpiecePointMoveRequested, this, &MainPage::forwardWorkpiecePointMove);
    connect(manualControl, &ManualControlWidget::currentPositionExportRequested, this, &MainPage::currentPositionExportRequested);
    connect(automaticOperation, &AutomaticOperationWidget::taskStateChangeRequested, this, &MainPage::taskStateChangeRequested);
    connect(automaticOperation, &AutomaticOperationWidget::workpiecePointQueuePrepared, this, &MainPage::prepareAutomaticMotorPointQueue);
    connect(automaticOperation, &AutomaticOperationWidget::taskTerminationRequested, this, &MainPage::taskTerminationRequested);
    return tabs;
}

void MainPage::applyCoordinateTransform(int index)
{
    if (index < 0 || index >= coordinateTransforms_.size()) {
        return;
    }

    workpieceToMotorTransform_ = coordinateTransforms_.at(index);
    transformParameters_->setText(coordinateTransformDescriptions_.at(index));
    updateAutomaticMotorPointQueue();
}

void MainPage::forwardWorkpiecePointMove(double workpieceX, double workpieceY)
{
    const QPointF motorTarget = workpieceToMotorTransform_.map(QPointF(workpieceX, workpieceY));
    emit workpiecePointMoveRequested(workpieceX, workpieceY, motorTarget.x(), motorTarget.y());
}

void MainPage::prepareAutomaticMotorPointQueue(
    const QString& taskType,
    const QList<WorkpieceTaskPoint>& points)
{
    automaticWorkpiecePointQueue_ = points;
    updateAutomaticMotorPointQueue();
    emit taskStateChangeRequested(
        QStringLiteral("点队列已生成"),
        QStringLiteral("已生成并转换 %1 个电机绝对 XY 目标点，等待下一步动作。")
            .arg(automaticTaskPointQueue_.size()),
        QStringLiteral("ready"));
    emit automaticTaskPrepared(taskType, automaticTaskPointQueue_);
}

void MainPage::updateAutomaticMotorPointQueue()
{
    automaticTaskPointQueue_.clear();
    automaticTaskPointQueue_.reserve(automaticWorkpiecePointQueue_.size());
    for (const WorkpieceTaskPoint& workpiecePoint : automaticWorkpiecePointQueue_) {
        automaticTaskPointQueue_.append(TaskExecutionPoint{
            workpiecePoint.position,
            workpieceToMotorTransform_.map(workpiecePoint.position),
            workpiecePoint.description});
    }
}

ManualControlWidget::ManualControlWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 18, 16, 16);
    layout->setSpacing(18);

    QHBoxLayout* movementLayout = new QHBoxLayout;
    movementLayout->setSpacing(18);
    movementLayout->addWidget(createAbsoluteMoveGroup(), 3);
    movementLayout->addWidget(createWorkpieceMoveGroup(), 2);
    layout->addLayout(movementLayout, 1);
    layout->addWidget(createPositionExportGroup());
}

QGroupBox* ManualControlWidget::createAbsoluteMoveGroup()
{
    QGroupBox* absoluteGroup = new QGroupBox(QStringLiteral("载物台绝对移动（电机坐标）"));
    QGridLayout* absoluteLayout = new QGridLayout(absoluteGroup);
    absoluteLayout->setHorizontalSpacing(12);
    absoluteLayout->setVerticalSpacing(12);
    absoluteLayout->addWidget(createColumnHeader(QStringLiteral("轴")), 0, 0);
    absoluteLayout->addWidget(createColumnHeader(QStringLiteral("绝对目标")), 0, 1);
    absoluteLayout->addWidget(createColumnHeader(QStringLiteral("单轴操作")), 0, 2);

    motorXInput_ = WidgetFactory::createCoordinateInput();
    motorYInput_ = WidgetFactory::createCoordinateInput();
    motorZInput_ = WidgetFactory::createCoordinateInput();
    QPushButton* moveX = new QPushButton(QStringLiteral("移动 X 轴"));
    QPushButton* moveY = new QPushButton(QStringLiteral("移动 Y 轴"));
    QPushButton* moveZ = new QPushButton(QStringLiteral("移动 Z 轴"));
    absoluteLayout->addWidget(new QLabel(QStringLiteral("X")), 1, 0);
    absoluteLayout->addWidget(motorXInput_, 1, 1);
    absoluteLayout->addWidget(moveX, 1, 2);
    absoluteLayout->addWidget(new QLabel(QStringLiteral("Y")), 2, 0);
    absoluteLayout->addWidget(motorYInput_, 2, 1);
    absoluteLayout->addWidget(moveY, 2, 2);
    absoluteLayout->addWidget(new QLabel(QStringLiteral("Z")), 3, 0);
    absoluteLayout->addWidget(motorZInput_, 3, 1);
    absoluteLayout->addWidget(moveZ, 3, 2);
    absoluteLayout->setColumnStretch(1, 1);

    QLabel* absoluteHint = new QLabel(QStringLiteral("输入电机 X/Y/Z 绝对目标，业务层校验后直接驱动载物台移动。"));
    absoluteHint->setWordWrap(true);
    absoluteHint->setProperty("role", "muted");
    absoluteLayout->addWidget(absoluteHint, 4, 0, 1, 3);
    QPushButton* absoluteMove = WidgetFactory::createPrimaryButton(QStringLiteral("XYZ 联动移动"));
    absoluteLayout->addWidget(absoluteMove, 5, 0, 1, 3);

    connect(moveX, &QPushButton::clicked, this, &ManualControlWidget::requestXAxisMove);
    connect(moveY, &QPushButton::clicked, this, &ManualControlWidget::requestYAxisMove);
    connect(moveZ, &QPushButton::clicked, this, &ManualControlWidget::requestZAxisMove);
    connect(absoluteMove, &QPushButton::clicked, this, &ManualControlWidget::requestStageMove);
    return absoluteGroup;
}

QGroupBox* ManualControlWidget::createWorkpieceMoveGroup()
{
    QGroupBox* workpieceGroup = new QGroupBox(QStringLiteral("物料测点引导（物料坐标）"));
    QFormLayout* workpieceLayout = new QFormLayout(workpieceGroup);
    workpieceXInput_ = WidgetFactory::createCoordinateInput();
    workpieceYInput_ = WidgetFactory::createCoordinateInput();
    workpieceLayout->addRow(QStringLiteral("物料 xw"), workpieceXInput_);
    workpieceLayout->addRow(QStringLiteral("物料 yw"), workpieceYInput_);
    QLabel* workpieceHint = new QLabel(QStringLiteral("输入客户提供的物料相对测点；业务层通过已示教坐标系转换为电机 X/Y 绝对目标。"));
    workpieceHint->setWordWrap(true);
    workpieceHint->setProperty("role", "muted");
    workpieceLayout->addRow(workpieceHint);
    QPushButton* workpieceMove = WidgetFactory::createPrimaryButton(QStringLiteral("引导测点到探头位置"));
    workpieceLayout->addRow(workpieceMove);

    connect(workpieceMove, &QPushButton::clicked, this, &ManualControlWidget::requestWorkpiecePointMove);
    return workpieceGroup;
}

QGroupBox* ManualControlWidget::createPositionExportGroup()
{
    QGroupBox* exportGroup = new QGroupBox(QStringLiteral("当前位置导出"));
    QHBoxLayout* exportLayout = new QHBoxLayout(exportGroup);
    QLabel* exportHint = new QLabel(QStringLiteral("导出当前电机绝对坐标 X/Y/Z 与物料相对坐标 xw/yw 快照。"));
    exportHint->setProperty("role", "muted");
    QPushButton* exportButton = new QPushButton(QStringLiteral("导出当前位置"));
    exportLayout->addWidget(exportHint);
    exportLayout->addStretch();
    exportLayout->addWidget(exportButton);
    connect(exportButton, &QPushButton::clicked, this, &ManualControlWidget::currentPositionExportRequested);
    return exportGroup;
}

void ManualControlWidget::requestXAxisMove()
{
    emit stageAxisAbsoluteMoveRequested(MotorAxis::X, motorXInput_->value());
}

void ManualControlWidget::requestYAxisMove()
{
    emit stageAxisAbsoluteMoveRequested(MotorAxis::Y, motorYInput_->value());
}

void ManualControlWidget::requestZAxisMove()
{
    emit stageAxisAbsoluteMoveRequested(MotorAxis::Z, motorZInput_->value());
}

void ManualControlWidget::requestStageMove()
{
    emit stageAbsoluteMoveRequested(
        motorXInput_->value(),
        motorYInput_->value(),
        motorZInput_->value());
}

void ManualControlWidget::requestWorkpiecePointMove()
{
    emit workpiecePointMoveRequested(workpieceXInput_->value(), workpieceYInput_->value());
}
