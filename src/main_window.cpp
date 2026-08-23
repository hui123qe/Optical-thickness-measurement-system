#include "main_window.h"

#include "motor/motion_controller_manager.h"

#include "device/device_manager.h"
#include "workflow/measurement_task_controller.h"
#include "workflow/task_executor.h"
#include "view/debug_page.h"
#include "view/log_page.h"
#include "view/operation_pages.h"
#include "view/status_widgets.h"

#include <limits>

#include <QHBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

Q_LOGGING_CATEGORY(mainWindowLog, "otms.ui.main_window")

namespace {

QString motorAxisName(MotorAxis axis)
{
    switch (axis) {
    case MotorAxis::X:
        return QStringLiteral("X");
    case MotorAxis::Y:
        return QStringLiteral("Y");
    case MotorAxis::Z:
        return QStringLiteral("Z");
    }
    return QStringLiteral("未知");
}

QString runModeName(otms::workflow::RunMode mode)
{
    return mode == otms::workflow::RunMode::Manual
        ? QStringLiteral("手动")
        : QStringLiteral("自动");
}

otms::device::LogicalAxis logicalAxis(MotorAxis axis)
{
    switch (axis) {
    case MotorAxis::X:
        return otms::device::LogicalAxis::X;
    case MotorAxis::Y:
        return otms::device::LogicalAxis::Y;
    case MotorAxis::Z:
        return otms::device::LogicalAxis::Z;
    }
    return otms::device::LogicalAxis::X;
}

MotorAxis motorAxis(otms::device::LogicalAxis axis)
{
    switch (axis) {
    case otms::device::LogicalAxis::X:
        return MotorAxis::X;
    case otms::device::LogicalAxis::Y:
        return MotorAxis::Y;
    case otms::device::LogicalAxis::Z:
        return MotorAxis::Z;
    }
    return MotorAxis::X;
}

QString taskExecutionStateName(
    TaskExecutor::ExecutionPhase phase,
    TaskExecutor::TaskResult lastResult)
{
    switch (phase) {
    case TaskExecutor::ExecutionPhase::Idle:
        switch (lastResult) {
        case TaskExecutor::TaskResult::None:
            return QStringLiteral("空闲");
        case TaskExecutor::TaskResult::Completed:
            return QStringLiteral("空闲（上一任务完成）");
        case TaskExecutor::TaskResult::Terminated:
            return QStringLiteral("空闲（上一任务终止）");
        case TaskExecutor::TaskResult::Failed:
            return QStringLiteral("空闲（上一任务失败）");
        }
        break;
    case TaskExecutor::ExecutionPhase::Moving:
        return QStringLiteral("正在移动");
    case TaskExecutor::ExecutionPhase::Measuring:
        return QStringLiteral("正在测量");
    case TaskExecutor::ExecutionPhase::Stopping:
        return QStringLiteral("正在停止");
    case TaskExecutor::ExecutionPhase::Faulted:
        return QStringLiteral("设备故障");
    }
    return QStringLiteral("未知状态");
}

QString taskExecutionStyleClass(
    TaskExecutor::ExecutionPhase phase,
    TaskExecutor::TaskResult lastResult)
{
    switch (phase) {
    case TaskExecutor::ExecutionPhase::Idle:
        if (lastResult == TaskExecutor::TaskResult::Completed) {
            return QStringLiteral("ready");
        }
        if (lastResult == TaskExecutor::TaskResult::Terminated
            || lastResult == TaskExecutor::TaskResult::Failed) {
            return QStringLiteral("terminated");
        }
        return QStringLiteral("waiting");
    case TaskExecutor::ExecutionPhase::Moving:
    case TaskExecutor::ExecutionPhase::Measuring:
        return QStringLiteral("ready");
    case TaskExecutor::ExecutionPhase::Stopping:
        return QStringLiteral("waiting");
    case TaskExecutor::ExecutionPhase::Faulted:
        return QStringLiteral("terminated");
    }
    return QStringLiteral("waiting");
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setObjectName(QStringLiteral("MainWindow"));
    setWindowTitle(QStringLiteral("光学厚度测量系统"));
    setMinimumSize(1280, 760);
    resize(1800, 1000);

    QWidget* root = new QWidget;
    root->setObjectName(QStringLiteral("body"));
    QVBoxLayout* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    topStatus_ = new TopStatusWidget;
    rootLayout->addWidget(topStatus_);

    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    NavigationWidget* navigation = new NavigationWidget;
    contentLayout->addWidget(navigation);

    QStackedWidget* pageStack = new QStackedWidget;
    pageStack->setObjectName(QStringLiteral("pageStack"));
    MainPage* mainPage = new MainPage;
    LogPage* logPage = new LogPage;
    LaserDebugPage* debugPage = new LaserDebugPage;
    pageStack->addWidget(mainPage);
    pageStack->addWidget(logPage);
    pageStack->addWidget(debugPage);
    contentLayout->addWidget(pageStack, 1);

    rightStatus_ = new RightStatusWidget;
    contentLayout->addWidget(rightStatus_);
    rootLayout->addLayout(contentLayout, 1);

    setCentralWidget(root);
    setStatusBar(new BottomStatusBar(this));
    taskExecutor_ = new TaskExecutor(this);

    otms::device::DeviceManager& deviceManager = otms::device::DeviceManager::instance();
    connect(&deviceManager, &otms::device::DeviceManager::laserConnectionChanged,
        this, [this, mainPage](bool connected, const QString&) {
            rightStatus_->setProbeConnectionState(connected);
            topStatus_->setLaserConnectionState(connected);
            mainPage->setLaserConnectionState(connected);
            taskExecutor_->setProbeReady(connected);
        });
    connect(&deviceManager, &otms::device::DeviceManager::laserMeasurementStateChanged,
        mainPage, &MainPage::setLaserMeasurementState);
    connect(&deviceManager, &otms::device::DeviceManager::laserMeasurementStateChanged,
        topStatus_, &TopStatusWidget::setLaserMeasurementState);
    connect(&deviceManager, &otms::device::DeviceManager::laserMeasurementUpdated,
        this, [this](const otms::device::LaserMeasurement& measurement) {
            const double measurementMillimeters =
                measurement.quality == otms::device::MeasurementQuality::Valid
                ? measurement.valueMicrometers / 1000.0
                : std::numeric_limits<double>::quiet_NaN();
            topStatus_->setLaserMeasurementMillimeters(measurementMillimeters);
        });
    const bool laserConnected = deviceManager.isLaserConnected();
    rightStatus_->setProbeConnectionState(laserConnected);
    topStatus_->setLaserConnectionState(laserConnected);
    mainPage->setLaserConnectionState(laserConnected);
    taskExecutor_->setProbeReady(laserConnected);

    otms::device::MotionControllerManager& motionControllers =
        otms::device::MotionControllerManager::instance();
    measurementTaskController_ = new MeasurementTaskController(
        *taskExecutor_,
        motionControllers,
        deviceManager,
        this);
    connect(measurementTaskController_, &MeasurementTaskController::motionConnectionChanged,
        rightStatus_, &RightStatusWidget::setMotionConnectionState);
    connect(measurementTaskController_, &MeasurementTaskController::motionConnectionChanged,
        topStatus_, &TopStatusWidget::setMotionConnectionState);
    connect(measurementTaskController_, &MeasurementTaskController::motorPositionChanged,
        topStatus_, &TopStatusWidget::setMotorPositionMillimeters);
    connect(measurementTaskController_, &MeasurementTaskController::axisEnableStateChanged,
        mainPage,
        [mainPage](otms::device::LogicalAxis axis, bool available, bool enabled) {
            mainPage->setAxisEnableState(motorAxis(axis), available, enabled);
        });
    connect(measurementTaskController_, &MeasurementTaskController::measurementAvailable,
        topStatus_, &TopStatusWidget::setLaserMeasurementMillimeters);
    connect(measurementTaskController_, &MeasurementTaskController::doorLockStateChanged,
        rightStatus_, &RightStatusWidget::setDoorLockState);
    connect(measurementTaskController_, &MeasurementTaskController::lightCurtainStateChanged,
        rightStatus_, &RightStatusWidget::setLightCurtainState);
    connect(measurementTaskController_, &MeasurementTaskController::machineStateChanged,
        rightStatus_, &RightStatusWidget::setMachineState);
    connect(measurementTaskController_, &MeasurementTaskController::initializationStateChanged,
        this,
        [this](otms::workflow::InitializationState state, const QString& detail) {
            topStatus_->setInitializationState(state);
            if (state == otms::workflow::InitializationState::Running) {
                setTaskState(
                    QStringLiteral("初始化中"),
                    detail,
                    QStringLiteral("waiting"));
            } else if (state == otms::workflow::InitializationState::Completed) {
                setTaskState(
                    QStringLiteral("初始化完成"),
                    detail,
                    QStringLiteral("ready"));
            } else if (state == otms::workflow::InitializationState::Failed) {
                setTaskState(
                    QStringLiteral("初始化失败"),
                    detail,
                    QStringLiteral("terminated"));
            }
        });
    connect(measurementTaskController_, &MeasurementTaskController::taskLogChanged,
        logPage, &LogPage::refresh);
    connect(measurementTaskController_, &MeasurementTaskController::errorOccurred,
        this, [this](const QString& detail) {
            qCWarning(mainWindowLog).noquote()
                << QStringLiteral("错误发生：%1").arg(detail);
            setTaskState(QStringLiteral("执行异常"), detail, QStringLiteral("terminated"));
        });
    connect(measurementTaskController_, &MeasurementTaskController::operationRejected,
        this, [this](const QString& detail) {
            QMessageBox::warning(
                this,
                QStringLiteral("无法执行操作"),
                detail);
        });
    connect(measurementTaskController_, &MeasurementTaskController::machineFaultOccurred,
        this, [this](const QString& detail) {
            QMessageBox::critical(
                this,
                QStringLiteral("机器故障报警"),
                detail);
        });
    connect(rightStatus_, &RightStatusWidget::motionConnectionRequested,
        this, &MainWindow::showMotionConnectionDialog);
    connect(rightStatus_, &RightStatusWidget::laserConnectionRequested,
        this, &MainWindow::showLaserConnectionDialog);
    connect(rightStatus_, &RightStatusWidget::runModeChangeRequested,
        this, [this](otms::workflow::RunMode targetMode) {
            const otms::workflow::MachineState machineState =
                measurementTaskController_->machineState();
            if (machineState == otms::workflow::MachineState::Running) {
                qCWarning(mainWindowLog)
                    << "Run mode change rejected: machine is running";
                QMessageBox::warning(
                    this,
                    QStringLiteral("无法切换运行模式"),
                    QStringLiteral("机器正在运行，请等待当前动作或任务结束。"));
                return;
            }
            if (machineState == otms::workflow::MachineState::Fault) {
                qCWarning(mainWindowLog)
                    << "Run mode change rejected: machine is faulted";
                QMessageBox::warning(
                    this,
                    QStringLiteral("无法切换运行模式"),
                    QStringLiteral("机器处于故障状态，请先完成故障复位。"));
                return;
            }

            const QString targetModeName = runModeName(targetMode);
            if (QMessageBox::question(
                    this,
                    QStringLiteral("切换运行模式"),
                    QStringLiteral("确认切换到%1模式吗？\n\n切换模式不会启动或停止设备。")
                        .arg(targetModeName),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No) != QMessageBox::Yes) {
                return;
            }

            if (measurementTaskController_->switchRunMode(targetMode)) {
                setTaskState(
                    QStringLiteral("运行模式已切换"),
                    QStringLiteral("当前为%1模式。").arg(targetModeName),
                    targetMode == otms::workflow::RunMode::Automatic
                        ? QStringLiteral("ready")
                        : QStringLiteral("waiting"));
            }
        });
    rightStatus_->setMachineState(
        measurementTaskController_->machineState(),
        measurementTaskController_->runMode());
    topStatus_->setInitializationState(
        measurementTaskController_->initializationState());

    connect(navigation, &NavigationWidget::pageSelected, this, [pageStack](int index) {
        if (index < 0 || index >= pageStack->count()) {
            return;
        }
        pageStack->setCurrentIndex(index);
    });
    connect(topStatus_, &TopStatusWidget::emergencyStopRequested, this, [this] {
        measurementTaskController_->emergencyStop();
        setTaskState(
            QStringLiteral("已急停"),
            QStringLiteral("软件故障已锁存；物理急停与安全联锁仍独立生效。"),
            QStringLiteral("terminated"));
    });
    connect(topStatus_, &TopStatusWidget::resetRequested, this, [this] {
        if (measurementTaskController_->resetMachineFault()) {
            setTaskState(
                QStringLiteral("故障已复位"),
                QStringLiteral("已清除软件故障锁存并重新评估启动条件。"),
                QStringLiteral("waiting"));
        }
    });
    connect(topStatus_, &TopStatusWidget::initializationRequested, this, [this] {
        if (QMessageBox::question(
                this,
                QStringLiteral("设备初始化确认"),
                QStringLiteral("确认执行设备初始化吗？\n\n将依次执行全轴使能和 XYZ 回零。"),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        measurementTaskController_->initializeMachine();
    });
    connect(topStatus_, &TopStatusWidget::doorLockCommandRequested,
        this, [this](bool locked) {
            const QString action = locked ? QStringLiteral("关锁") : QStringLiteral("开锁");
            if (QMessageBox::question(
                    this,
                    QStringLiteral("门锁控制确认"),
                    QStringLiteral("确认要%1吗？").arg(action),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No) != QMessageBox::Yes) {
                return;
            }

            if (measurementTaskController_->setDoorLocked(locked)) {
                setTaskState(
                    QStringLiteral("门锁命令已发送"),
                    QStringLiteral("已发送%1命令。").arg(action),
                    QStringLiteral("waiting"));
            }
    });
    connect(mainPage, &MainPage::taskStateChangeRequested, this, &MainWindow::setTaskState);
    connect(mainPage, &MainPage::automaticTaskPrepared,
        measurementTaskController_, &MeasurementTaskController::startAutomaticTask);
    connect(mainPage, &MainPage::stageHomeRequested, this, [this] {
        if (measurementTaskController_->homeStage()) {
            setTaskState(
                QStringLiteral("回零中"),
                QStringLiteral("X、Y、Z 轴正在执行回零。"),
                QStringLiteral("ready"));
        }
    });
    connect(mainPage, &MainPage::stageAxisHomeRequested, this,
        [this](MotorAxis axis) {
            if (measurementTaskController_->homeAxis(logicalAxis(axis))) {
                setTaskState(
                    QStringLiteral("回零中"),
                    QStringLiteral("%1 轴正在执行回零。").arg(motorAxisName(axis)),
                    QStringLiteral("ready"));
            }
        });
    connect(mainPage, &MainPage::stageAxisEnableRequested, this,
        [this](MotorAxis axis, bool enabled) {
            if (measurementTaskController_->setAxisEnabled(logicalAxis(axis), enabled)) {
                setTaskState(
                    enabled ? QStringLiteral("轴已使能") : QStringLiteral("轴已去使能"),
                    QStringLiteral("%1 轴已%2。")
                        .arg(
                            motorAxisName(axis),
                            enabled ? QStringLiteral("使能") : QStringLiteral("去使能")),
                    enabled ? QStringLiteral("ready") : QStringLiteral("waiting"));
            }
        });
    connect(mainPage, &MainPage::taskTerminationRequested, taskExecutor_, &TaskExecutor::terminate);
    connect(mainPage, &MainPage::laserMeasurementStartRequested, this, [this, &deviceManager] {
        const otms::device::LaserStatus status = deviceManager.startLaserMeasurement();
        setTaskState(
            status.ok() ? QStringLiteral("测量已启动") : QStringLiteral("无法启动测量"),
            status.message,
            status.ok() ? QStringLiteral("ready") : QStringLiteral("terminated"));
    });
    connect(mainPage, &MainPage::laserMeasurementStopRequested, this, [this, &deviceManager] {
        const otms::device::LaserStatus status = deviceManager.stopLaserMeasurement();
        setTaskState(
            status.ok() ? QStringLiteral("测量已停止") : QStringLiteral("无法停止测量"),
            status.message,
            status.ok() ? QStringLiteral("waiting") : QStringLiteral("terminated"));
    });
    const auto updateTaskExecutionState = [this](const QString& detail) {
        setTaskState(
            taskExecutionStateName(taskExecutor_->phase(), taskExecutor_->lastResult()),
            detail,
            taskExecutionStyleClass(taskExecutor_->phase(), taskExecutor_->lastResult()));
    };
    connect(taskExecutor_, &TaskExecutor::phaseChanged, this,
        [updateTaskExecutionState](TaskExecutor::ExecutionPhase, const QString& detail) {
            updateTaskExecutionState(detail);
        });
    connect(taskExecutor_, &TaskExecutor::taskResultChanged, this,
        [updateTaskExecutionState](TaskExecutor::TaskResult, const QString& detail) {
            updateTaskExecutionState(detail);
        });
    connect(taskExecutor_, &TaskExecutor::executionRejected, this, [this](const QString& reason) {
        qCWarning(mainWindowLog).noquote()
            << QStringLiteral("执行器拒绝：%1").arg(reason);
        setTaskState(QStringLiteral("无法执行"), reason, QStringLiteral("terminated"));
        QMessageBox::warning(this, QStringLiteral("无法执行操作"), reason);
    });
    connect(mainPage, &MainPage::stageAxisAbsoluteMoveRequested, this, [this](MotorAxis axis, double target) {
        if (measurementTaskController_->moveAxisAbsolute(logicalAxis(axis), target)) {
            setTaskState(
                QStringLiteral("手动运动中"),
                QStringLiteral("电机 %1 正在绝对定位至 %2 mm。")
                    .arg(motorAxisName(axis))
                    .arg(target, 0, 'f', 3),
                QStringLiteral("ready"));
        }
    });
    connect(mainPage, &MainPage::stageAxisRelativeMoveRequested, this, [this](MotorAxis axis, double distance) {
        if (measurementTaskController_->moveAxisRelative(logicalAxis(axis), distance)) {
            setTaskState(
                QStringLiteral("手动运动中"),
                QStringLiteral("电机 %1 正在相对移动 %2 mm。")
                    .arg(motorAxisName(axis))
                    .arg(distance, 0, 'f', 3),
                QStringLiteral("ready"));
        }
    });
    connect(mainPage, &MainPage::stageAbsoluteMoveRequested, this, [this](double motorX, double motorY, double motorZ) {
        if (measurementTaskController_->moveStageAbsolute(motorX, motorY, motorZ)) {
            setTaskState(
                QStringLiteral("手动运动中"),
                QStringLiteral("载物台正在移动至 X=%1 mm，Y=%2 mm，Z=%3 mm。")
                    .arg(motorX, 0, 'f', 3)
                    .arg(motorY, 0, 'f', 3)
                    .arg(motorZ, 0, 'f', 3),
                QStringLiteral("ready"));
        }
    });
    connect(mainPage, &MainPage::currentPositionExportRequested, this, [this] {
        showPrototypeNotice(QStringLiteral("导出当前电机绝对坐标与物料相对坐标"));
    });
    connect(mainPage, &MainPage::workpiecePointMoveRequested, this, [this](double workpieceX, double workpieceY, double motorX, double motorY) {
        if (measurementTaskController_->moveWorkpiecePoint(motorX, motorY)) {
            setTaskState(
                QStringLiteral("手动运动中"),
                QStringLiteral("物料测点 (%1, %2) 正在移动至电机坐标 (%3, %4) mm。")
                    .arg(workpieceX, 0, 'f', 3)
                    .arg(workpieceY, 0, 'f', 3)
                    .arg(motorX, 0, 'f', 3)
                    .arg(motorY, 0, 'f', 3),
                QStringLiteral("ready"));
        }
    });
    connect(logPage, &LogPage::exportRequested, this, [this] {
        showPrototypeNotice(QStringLiteral("导出选中完成记录"));
    });
    navigation->selectPage(0);
}

void MainWindow::showMotionConnectionDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("运动控制器连接"));
    dialog.setMinimumWidth(420);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    const QStringList controllerIds = measurementTaskController_->motionControllerIds();
    if (controllerIds.isEmpty()) {
        QLabel* empty = new QLabel(QStringLiteral("当前没有已配置的运动控制器。"));
        empty->setProperty("role", "muted");
        layout->addWidget(empty);
    }

    for (const QString& controllerId : controllerIds) {
        QGroupBox* controllerGroup = new QGroupBox(controllerId);
        QHBoxLayout* controllerLayout = new QHBoxLayout(controllerGroup);
        QLabel* connectionState = new QLabel;
        connectionState->setMinimumWidth(72);
        const auto refreshState = [this, controllerId, connectionState] {
            connectionState->setText(
                measurementTaskController_->isMotionControllerConnected(controllerId)
                    ? QStringLiteral("已连接")
                    : QStringLiteral("未连接"));
        };
        refreshState();

        QPushButton* connectButton = new QPushButton(QStringLiteral("连接"));
        QPushButton* disconnectButton = new QPushButton(QStringLiteral("断开"));
        controllerLayout->addWidget(new QLabel(QStringLiteral("连接状态")));
        controllerLayout->addWidget(connectionState);
        controllerLayout->addStretch();
        controllerLayout->addWidget(connectButton);
        controllerLayout->addWidget(disconnectButton);

        connect(connectButton, &QPushButton::clicked, &dialog,
            [this, controllerId, refreshState] {
                if (measurementTaskController_->connectMotionController(controllerId)) {
                    setTaskState(
                        QStringLiteral("设备连接"),
                        QStringLiteral("运动控制器 %1 已执行连接命令。").arg(controllerId),
                        QStringLiteral("ready"));
                }
                refreshState();
            });
        connect(disconnectButton, &QPushButton::clicked, &dialog,
            [this, controllerId, refreshState] {
                if (measurementTaskController_->disconnectMotionController(controllerId)) {
                    setTaskState(
                        QStringLiteral("设备断开"),
                        QStringLiteral("运动控制器 %1 已执行断开命令。").arg(controllerId),
                        QStringLiteral("waiting"));
                }
                refreshState();
            });
        layout->addWidget(controllerGroup);
    }

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}

void MainWindow::showLaserConnectionDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("激光探头连接"));
    dialog.setMinimumWidth(420);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QGroupBox* laserGroup = new QGroupBox(QStringLiteral("激光探头"));
    QHBoxLayout* laserLayout = new QHBoxLayout(laserGroup);
    QLabel* connectionState = new QLabel;
    connectionState->setMinimumWidth(72);
    const auto refreshState = [this, connectionState] {
        connectionState->setText(
            measurementTaskController_->isLaserConnected()
                ? QStringLiteral("已连接")
                : QStringLiteral("未连接"));
    };
    refreshState();

    QPushButton* connectButton = new QPushButton(QStringLiteral("连接"));
    QPushButton* disconnectButton = new QPushButton(QStringLiteral("断开"));
    laserLayout->addWidget(new QLabel(QStringLiteral("连接状态")));
    laserLayout->addWidget(connectionState);
    laserLayout->addStretch();
    laserLayout->addWidget(connectButton);
    laserLayout->addWidget(disconnectButton);
    layout->addWidget(laserGroup);

    connect(connectButton, &QPushButton::clicked, &dialog, [this, refreshState] {
        if (measurementTaskController_->connectLaser()) {
            setTaskState(
                QStringLiteral("设备连接"),
                QStringLiteral("激光探头已执行连接命令。"),
                QStringLiteral("ready"));
        }
        refreshState();
    });
    connect(disconnectButton, &QPushButton::clicked, &dialog, [this, refreshState] {
        if (measurementTaskController_->disconnectLaser()) {
            setTaskState(
                QStringLiteral("设备断开"),
                QStringLiteral("激光探头已执行断开命令。"),
                QStringLiteral("waiting"));
        }
        refreshState();
    });

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
}

void MainWindow::setTaskState(
    const QString& state,
    const QString& detail,
    const QString&)
{
    statusBar()->showMessage(
        detail.isEmpty() ? state : QStringLiteral("%1：%2").arg(state, detail),
        5000);
}

void MainWindow::showPrototypeNotice(const QString& action)
{
    setTaskState(QStringLiteral("等待接入"), QStringLiteral("%1意图未发送：界面原型尚未连接业务服务。").arg(action), QStringLiteral("waiting"));
    QMessageBox::information(this, QStringLiteral("业务意图"), action + QStringLiteral("\n\n界面尚未连接设备或业务服务，不会产生硬件动作。"));
}
