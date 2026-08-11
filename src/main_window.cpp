#include "main_window.h"

#include "device/device_manager.h"
#include "workflow/measurement_task_controller.h"
#include "workflow/task_executor.h"
#include "view/debug_page.h"
#include "view/log_page.h"
#include "view/operation_pages.h"
#include "view/status_widgets.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

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
    resize(1520, 920);

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
    measurementTaskController_ = new MeasurementTaskController(*taskExecutor_, this);
    connect(measurementTaskController_, &MeasurementTaskController::motionConnectionChanged,
        rightStatus_, &RightStatusWidget::setMotionConnectionState);
    connect(measurementTaskController_, &MeasurementTaskController::motionConnectionChanged,
        topStatus_, &TopStatusWidget::setMotionConnectionState);
    connect(measurementTaskController_, &MeasurementTaskController::motorPositionChanged,
        topStatus_, &TopStatusWidget::setMotorPositionMillimeters);
    connect(measurementTaskController_, &MeasurementTaskController::measurementAvailable,
        topStatus_, &TopStatusWidget::setLaserMeasurementMillimeters);
    connect(measurementTaskController_, &MeasurementTaskController::taskLogChanged,
        logPage, &LogPage::refresh);
    connect(measurementTaskController_, &MeasurementTaskController::errorOccurred,
        this, [this](const QString& detail) {
            setTaskState(QStringLiteral("执行异常"), detail, QStringLiteral("terminated"));
        });

    connect(navigation, &NavigationWidget::pageSelected, this, [pageStack](int index) {
        if (index < 0 || index >= pageStack->count()) {
            return;
        }
        pageStack->setCurrentIndex(index);
    });
    connect(topStatus_, &TopStatusWidget::emergencyStopRequested, this, [this] {
        taskExecutor_->terminate();
        setTaskState(QStringLiteral("已急停"), QStringLiteral("已提交软件急停意图；物理急停与安全联锁仍独立生效。"), QStringLiteral("terminated"));
    });
    connect(mainPage, &MainPage::taskStateChangeRequested, this, &MainWindow::setTaskState);
    connect(mainPage, &MainPage::automaticTaskPrepared, taskExecutor_, &TaskExecutor::start);
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
        setTaskState(QStringLiteral("无法执行"), reason, QStringLiteral("terminated"));
    });
    connect(mainPage, &MainPage::stageAxisAbsoluteMoveRequested, this, [this](MotorAxis axis, double target) {
        const QString action = QStringLiteral("载物台单轴绝对移动：电机 %1=%2 mm")
                                   .arg(motorAxisName(axis))
                                   .arg(target, 0, 'f', 3);
        showPrototypeNotice(action);
    });
    connect(mainPage, &MainPage::stageAbsoluteMoveRequested, this, [this](double motorX, double motorY, double motorZ) {
        const QString action = QStringLiteral("载物台绝对移动：电机 X=%1 mm，Y=%2 mm，Z=%3 mm")
                                   .arg(motorX, 0, 'f', 3)
                                   .arg(motorY, 0, 'f', 3)
                                   .arg(motorZ, 0, 'f', 3);
        showPrototypeNotice(action);
    });
    connect(mainPage, &MainPage::currentPositionExportRequested, this, [this] {
        showPrototypeNotice(QStringLiteral("导出当前电机绝对坐标与物料相对坐标"));
    });
    connect(mainPage, &MainPage::workpiecePointMoveRequested, this, [this](double workpieceX, double workpieceY, double motorX, double motorY) {
        const QString action = QStringLiteral("物料测点引导：xw=%1 mm，yw=%2 mm → 电机 X=%3 mm，Y=%4 mm")
                                   .arg(workpieceX, 0, 'f', 3)
                                   .arg(workpieceY, 0, 'f', 3)
                                   .arg(motorX, 0, 'f', 3)
                                   .arg(motorY, 0, 'f', 3);
        showPrototypeNotice(action);
    });
    connect(logPage, &LogPage::exportRequested, this, [this] {
        showPrototypeNotice(QStringLiteral("导出选中完成记录"));
    });
    navigation->selectPage(0);
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
