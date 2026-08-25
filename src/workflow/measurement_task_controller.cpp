#include "measurement_task_controller.h"

#include "../device/device_manager.h"
#include "../motor/motion_controller_manager.h"

#include <cmath>
#include <cstdint>

#include <QPointF>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(measurementWorkflowLog, "otms.workflow.measurement")

namespace {

constexpr int ArrivalPollIntervalMs = 20;
constexpr int MotorStatusPollIntervalMs = 200;
constexpr qint64 ArrivalTimeoutMs = 10000;
constexpr double ArrivalToleranceMillimeters = 0.001;
constexpr int RequiredArrivalSamples = 2;
constexpr unsigned int XAxisMask = 1U << 0U;
constexpr unsigned int YAxisMask = 1U << 1U;
constexpr unsigned int ZAxisMask = 1U << 2U;

const char* machineStateName(otms::workflow::MachineState state)
{
    switch (state) {
    case otms::workflow::MachineState::NotReady:
        return "NotReady";
    case otms::workflow::MachineState::Ready:
        return "Ready";
    case otms::workflow::MachineState::Running:
        return "Running";
    case otms::workflow::MachineState::Fault:
        return "Fault";
    }
    return "Unknown";
}

const char* runModeName(otms::workflow::RunMode mode)
{
    switch (mode) {
    case otms::workflow::RunMode::Manual:
        return "Manual";
    case otms::workflow::RunMode::Automatic:
        return "Automatic";
    }
    return "Unknown";
}

QString axisName(otms::device::LogicalAxis axis)
{
    switch (axis) {
    case otms::device::LogicalAxis::X:
        return QStringLiteral("X");
    case otms::device::LogicalAxis::Y:
        return QStringLiteral("Y");
    case otms::device::LogicalAxis::Z:
        return QStringLiteral("Z");
    }
    return QStringLiteral("未知");
}

QString logRunId(const QString& runId)
{
    return runId.left(8);
}

QString taskResultName(TaskExecutor::TaskResult result)
{
    switch (result) {
    case TaskExecutor::TaskResult::Completed:
        return QStringLiteral("完成");
    case TaskExecutor::TaskResult::Terminated:
        return QStringLiteral("终止");
    case TaskExecutor::TaskResult::Failed:
        return QStringLiteral("失败");
    case TaskExecutor::TaskResult::None:
        break;
    }
    return QStringLiteral("未知");
}

} // namespace

MeasurementTaskController::MeasurementTaskController(
    otms::workflow::WorkflowPolicy& workflowPolicy,
    TaskExecutor& executor,
    otms::device::MotionControllerManager& motionControllers,
    otms::device::DeviceManager& devices,
    QObject* parent)
    : QObject(parent)
    , workflowPolicy_(workflowPolicy)
    , executor_(executor)
    , motionControllers_(motionControllers)
    , devices_(devices)
{
    arrivalPollTimer_.setInterval(ArrivalPollIntervalMs);
    arrivalPollTimer_.setTimerType(Qt::PreciseTimer);
    motorStatusPollTimer_.setInterval(MotorStatusPollIntervalMs);
    motorStatusPollTimer_.setTimerType(Qt::CoarseTimer);

    connect(&arrivalPollTimer_, &QTimer::timeout, this, &MeasurementTaskController::pollArrival);
    connect(&motorStatusPollTimer_, &QTimer::timeout,
        this, &MeasurementTaskController::pollMotorStatus);
    connect(&executor_, &TaskExecutor::executionStarted,
        this, &MeasurementTaskController::prepareExperiment);
    connect(&executor_, &TaskExecutor::moveRequested,
        this, &MeasurementTaskController::beginMove);
    connect(&executor_, &TaskExecutor::measurementRequested,
        this, &MeasurementTaskController::measurePoint);
    connect(&executor_, &TaskExecutor::motionStopRequested,
        this, &MeasurementTaskController::stopMotion);
    connect(&executor_, &TaskExecutor::probeStopRequested,
        this, &MeasurementTaskController::stopProbe);
    connect(&executor_, &TaskExecutor::taskFinished,
        this, &MeasurementTaskController::persistTaskLog);
    connect(&executor_, &TaskExecutor::executionStarted,
        this, [this](const QString&, quint64, const QString&, int) {
            setMachineState(otms::workflow::MachineState::Running);
        });
    connect(&executor_, &TaskExecutor::executionCompleted,
        this, [this](quint64, int) { finishRunning(); });
    connect(&executor_, &TaskExecutor::executionTerminated,
        this, [this](quint64) { finishRunning(); });
    connect(&executor_, &TaskExecutor::executionFailed,
        this, [this](quint64, const QString& reason) { handleTaskFailure(reason); });
    connect(&devices_, &otms::device::DeviceManager::laserConnectionChanged,
        this, [this](bool connected, const QString&) {
            probeReady_ = connected;
            executor_.setProbeReady(databaseReady_ && probeReady_);
        });

    initializeDatabase();
    probeReady_ = devices_.isLaserConnected();
    executor_.setProbeReady(databaseReady_ && probeReady_);
    motorStatusPollTimer_.start();
}

otms::workflow::MachineState MeasurementTaskController::machineState() const
{
    return machineState_;
}

otms::workflow::RunMode MeasurementTaskController::runMode() const
{
    return runMode_;
}

otms::workflow::InitializationState MeasurementTaskController::initializationState() const
{
    return initializationState_;
}

otms::workflow::OperationProfile MeasurementTaskController::operationProfile() const
{
    return workflowPolicy_.profile();
}

QStringList MeasurementTaskController::motionControllerIds() const
{
    return motionControllers_.controllerIds();
}

bool MeasurementTaskController::isMotionControllerConnected(const QString& controllerId) const
{
    int connected = 0;
    return motionControllers_.isControllerConnected(controllerId, connected) == 1
        && connected != 0;
}

bool MeasurementTaskController::isLaserConnected() const
{
    return devices_.isLaserConnected();
}

bool MeasurementTaskController::setDoorLocked(bool locked)
{
    qCInfo(measurementWorkflowLog) << "Door lock command requested" << locked;
    const int result = motionControllers_.setDoorLocked(locked);
    if (result != 1) {
        emit errorOccurred(
            locked ? QStringLiteral("发送门锁锁定命令失败。")
                   : QStringLiteral("发送门锁解锁命令失败。"));
        return false;
    }
    return true;
}

bool MeasurementTaskController::connectMotionController(const QString& controllerId)
{
    qCInfo(measurementWorkflowLog) << "Motion controller connect requested" << controllerId;
    if (motionControllers_.connectController(controllerId) != 1) {
        emit errorOccurred(QStringLiteral("运动控制器 %1 连接失败。").arg(controllerId));
        return false;
    }
    pollMotorStatus();
    return true;
}

bool MeasurementTaskController::disconnectMotionController(const QString& controllerId)
{
    qCWarning(measurementWorkflowLog) << "Motion controller disconnect requested" << controllerId;
    if (motionControllers_.disconnectController(controllerId) != 1) {
        emit errorOccurred(QStringLiteral("运动控制器 %1 断开失败。").arg(controllerId));
        return false;
    }
    pollMotorStatus();
    return true;
}

bool MeasurementTaskController::connectLaser()
{
    qCInfo(measurementWorkflowLog) << "Laser connection requested";
    const otms::device::LaserStatus status = devices_.connectLaserEthernet();
    if (!status.ok()) {
        emit errorOccurred(status.message);
        return false;
    }
    updateStateFromConditions();
    return true;
}

bool MeasurementTaskController::disconnectLaser()
{
    qCWarning(measurementWorkflowLog) << "Laser disconnection requested";
    const otms::device::LaserStatus status = devices_.disconnectLaser();
    if (!status.ok()) {
        emit errorOccurred(status.message);
        return false;
    }
    updateStateFromConditions();
    return true;
}

bool MeasurementTaskController::switchRunMode(otms::workflow::RunMode mode)
{
    if (mode == runMode_) {
        return true;
    }
    if (executor_.phase() == TaskExecutor::ExecutionPhase::Moving
        || executor_.phase() == TaskExecutor::ExecutionPhase::Measuring
        || executor_.phase() == TaskExecutor::ExecutionPhase::Stopping) {
        rejectOperation(QStringLiteral("自动任务正在运行，无法切换运行模式。"));
        return false;
    }

    setRunMode(mode);
    return true;
}

bool MeasurementTaskController::switchOperationProfile(
    otms::workflow::OperationProfile profile)
{
    if (workflowPolicy_.profile() == profile) {
        return true;
    }

    workflowPolicy_.setProfile(profile);
    qCWarning(measurementWorkflowLog)
        << "Operation profile changed"
        << static_cast<int>(profile);
    emit operationProfileChanged(profile);
    return true;
}

bool MeasurementTaskController::initializeMachine()
{
    qCInfo(measurementWorkflowLog) << "Machine initialization requested";
    if (!operationAllowed(otms::workflow::OperationKind::Initialization)) {
        return false;
    }

    setMachineState(otms::workflow::MachineState::NotReady);
    setInitializationState(
        otms::workflow::InitializationState::Running,
        QStringLiteral("正在执行全轴使能。"));

    const int xEnableResult = motionControllers_.enableAxis(otms::device::LogicalAxis::X);
    const int yEnableResult = motionControllers_.enableAxis(otms::device::LogicalAxis::Y);
    const int zEnableResult = motionControllers_.enableAxis(otms::device::LogicalAxis::Z);
    if (xEnableResult != 1 || yEnableResult != 1 || zEnableResult != 1) {
        failInitialization(QStringLiteral("全轴使能失败。"));
        return false;
    }

    setInitializationState(
        otms::workflow::InitializationState::Running,
        QStringLiteral("全轴使能完成，正在执行 XYZ 回零。"));
    const int xHomeResult = motionControllers_.homeAxis(otms::device::LogicalAxis::X, true);
    const int yHomeResult = motionControllers_.homeAxis(otms::device::LogicalAxis::Y, true);
    const int zHomeResult = motionControllers_.homeAxis(otms::device::LogicalAxis::Z, true);
    if (xHomeResult != 1 || yHomeResult != 1 || zHomeResult != 1) {
        abortAllAxes();
        failInitialization(QStringLiteral("XYZ 回零命令失败。"));
        return false;
    }

    initializationMotionAxisMask_ = XAxisMask | YAxisMask | ZAxisMask;
    return true;
}

otms::workflow::WorkflowStateSnapshot MeasurementTaskController::workflowStateSnapshot() const
{
    const TaskExecutor::ExecutionPhase executionPhase = executor_.phase();
    otms::workflow::WorkflowStateSnapshot state;
    state.machineState = machineState_;
    state.runMode = runMode_;
    state.initializationState = initializationState_;
    state.automaticTaskInProgress =
        executionPhase == TaskExecutor::ExecutionPhase::Moving
        || executionPhase == TaskExecutor::ExecutionPhase::Measuring
        || executionPhase == TaskExecutor::ExecutionPhase::Stopping;
    state.motionConnected = motionConnected_;
    state.safetyConditionsMet = safetyConditionsMet();
    state.safetyFailureReason = safetyConditionFailureReason();
    return state;
}

bool MeasurementTaskController::operationAllowed(
    otms::workflow::OperationKind operation)
{
    const otms::workflow::OperationDecision decision =
        workflowPolicy_.evaluate(operation, workflowStateSnapshot());
    if (!decision.allowed) {
        rejectOperation(decision.rejectionReason);
        return false;
    }

    if (decision.bypassed) {
        qCWarning(measurementWorkflowLog).noquote()
            << QStringLiteral("调试旁路放行操作 %1，已忽略：%2")
                   .arg(static_cast<int>(operation))
                   .arg(decision.bypassedConditions.join(QStringLiteral("、")));
    }
    return true;
}

bool MeasurementTaskController::homeAxis(otms::device::LogicalAxis axis)
{
    qCInfo(measurementWorkflowLog) << "Manual axis home requested" << axisName(axis);
    if (!operationAllowed(otms::workflow::OperationKind::ManualMotion)) {
        return false;
    }
    if (motionControllers_.homeAxis(axis, true) != 1) {
        reportOperationFailure(QStringLiteral("%1 轴回零命令失败。").arg(axisName(axis)));
        return false;
    }

    return true;
}

bool MeasurementTaskController::homeStage()
{
    qCInfo(measurementWorkflowLog) << "Manual XYZ home requested";
    if (!operationAllowed(otms::workflow::OperationKind::ManualMotion)) {
        return false;
    }

    const int xResult = motionControllers_.homeAxis(otms::device::LogicalAxis::X, true);
    const int yResult = motionControllers_.homeAxis(otms::device::LogicalAxis::Y, true);
    const int zResult = motionControllers_.homeAxis(otms::device::LogicalAxis::Z, true);
    if (xResult != 1 || yResult != 1 || zResult != 1) {
        reportOperationFailure(QStringLiteral("XYZ 轴回零命令失败。"));
        return false;
    }

    return true;
}

bool MeasurementTaskController::setAxisEnabled(
    otms::device::LogicalAxis axis,
    bool enabled)
{
    qCInfo(measurementWorkflowLog)
        << "Manual axis enable command requested" << axisName(axis) << enabled;
    if (!operationAllowed(otms::workflow::OperationKind::ManualAxisEnable)) {
        return false;
    }

    const int result = enabled
        ? motionControllers_.enableAxis(axis)
        : motionControllers_.disableAxis(axis);
    if (result != 1) {
        rejectOperation(
            QStringLiteral("%1 轴%2命令失败。")
                .arg(axisName(axis), enabled ? QStringLiteral("使能") : QStringLiteral("去使能")));
        return false;
    }

    pollMotorStatus();
    return true;
}

bool MeasurementTaskController::moveAxisAbsolute(
    otms::device::LogicalAxis axis,
    double position)
{
    if (!operationAllowed(otms::workflow::OperationKind::ManualMotion)) {
        return false;
    }
    if (motionControllers_.moveAbsolute(axis, position, true) != 1) {
        reportOperationFailure(QStringLiteral("手动绝对定位命令失败。"));
        return false;
    }

    return true;
}

bool MeasurementTaskController::moveAxisRelative(
    otms::device::LogicalAxis axis,
    double distance)
{
    if (!operationAllowed(otms::workflow::OperationKind::ManualMotion)) {
        return false;
    }
    if (motionControllers_.moveRelative(axis, distance, true) != 1) {
        reportOperationFailure(QStringLiteral("手动相对移动命令失败。"));
        return false;
    }

    return true;
}

bool MeasurementTaskController::moveStageAbsolute(
    double xPosition,
    double yPosition,
    double zPosition)
{
    if (!operationAllowed(otms::workflow::OperationKind::ManualMotion)) {
        return false;
    }

    const int xResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::X, xPosition, true);
    const int yResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::Y, yPosition, true);
    const int zResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::Z, zPosition, true);
    if (xResult != 1 || yResult != 1 || zResult != 1) {
        reportOperationFailure(QStringLiteral("XYZ 联动绝对定位命令失败。"));
        return false;
    }

    return true;
}

bool MeasurementTaskController::moveWorkpiecePoint(double xPosition, double yPosition)
{
    if (!operationAllowed(otms::workflow::OperationKind::ManualMotion)) {
        return false;
    }

    const int xResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::X, xPosition, true);
    const int yResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::Y, yPosition, true);
    if (xResult != 1 || yResult != 1) {
        reportOperationFailure(QStringLiteral("物料测点手动定位命令失败。"));
        return false;
    }

    return true;
}

void MeasurementTaskController::startAutomaticTask(
    const QString& taskType,
    const QList<TaskExecutionPoint>& points)
{
    if (!operationAllowed(otms::workflow::OperationKind::AutomaticTask)) {
        return;
    }
    executor_.start(taskType, points);
}

void MeasurementTaskController::emergencyStop()
{
    enterMachineFault(QStringLiteral("软件急停已触发。"));
}

bool MeasurementTaskController::resetMachineFault()
{
    if (machineState_ != otms::workflow::MachineState::Fault) {
        rejectOperation(QStringLiteral("机器当前不处于故障状态，无需复位。"));
        return false;
    }
    if (executor_.phase() == TaskExecutor::ExecutionPhase::Moving
        || executor_.phase() == TaskExecutor::ExecutionPhase::Measuring
        || executor_.phase() == TaskExecutor::ExecutionPhase::Stopping) {
        rejectOperation(QStringLiteral("自动任务尚未完全停止，请稍后复位。"));
        return false;
    }
    if (executor_.phase() == TaskExecutor::ExecutionPhase::Faulted) {
        executor_.resetFault();
    }

    initializationMotionAxisMask_ = 0U;
    setMachineState(
        startupConditionsMet()
            ? otms::workflow::MachineState::Ready
            : otms::workflow::MachineState::NotReady);
    return true;
}

void MeasurementTaskController::pollMotorStatus()
{
    int xConnected = 0;
    int yConnected = 0;
    int zConnected = 0;
    const bool connected = motionControllers_.isAxisControllerConnected(
                               otms::device::LogicalAxis::X,
                               xConnected) == 1
        && motionControllers_.isAxisControllerConnected(
               otms::device::LogicalAxis::Y,
               yConnected) == 1
        && motionControllers_.isAxisControllerConnected(
               otms::device::LogicalAxis::Z,
               zConnected) == 1
        && xConnected != 0
        && yConnected != 0
        && zConnected != 0;

    double xPosition = 0.0;
    double yPosition = 0.0;
    double zPosition = 0.0;
    const bool positionAvailable = connected
        && motionControllers_.getPosition(otms::device::LogicalAxis::X, xPosition) == 1
        && motionControllers_.getPosition(otms::device::LogicalAxis::Y, yPosition) == 1
        && motionControllers_.getPosition(otms::device::LogicalAxis::Z, zPosition) == 1;
    int xEnabled = 0;
    int yEnabled = 0;
    int zEnabled = 0;
    const bool xEnableStateAvailable = connected
        && motionControllers_.isAxisEnabled(otms::device::LogicalAxis::X, xEnabled) == 1;
    const bool yEnableStateAvailable = connected
        && motionControllers_.isAxisEnabled(otms::device::LogicalAxis::Y, yEnabled) == 1;
    const bool zEnableStateAvailable = connected
        && motionControllers_.isAxisEnabled(otms::device::LogicalAxis::Z, zEnabled) == 1;
    axesEnabled_ = xEnableStateAvailable
        && yEnableStateAvailable
        && zEnableStateAvailable
        && xEnabled != 0
        && yEnabled != 0
        && zEnabled != 0;
    const bool motionAvailable = positionAvailable && axesEnabled_;

    const auto publishAxisEnableState =
        [this](
            otms::device::LogicalAxis axis,
            bool available,
            bool enabled,
            bool& previousAvailable,
            bool& previousEnabled) {
            if (available != previousAvailable
                || (available && enabled != previousEnabled)) {
                previousAvailable = available;
                previousEnabled = enabled;
                emit axisEnableStateChanged(axis, available, enabled);
            }
        };
    publishAxisEnableState(
        otms::device::LogicalAxis::X,
        xEnableStateAvailable,
        xEnabled != 0,
        xAxisEnableStateAvailable_,
        xAxisEnabled_);
    publishAxisEnableState(
        otms::device::LogicalAxis::Y,
        yEnableStateAvailable,
        yEnabled != 0,
        yAxisEnableStateAvailable_,
        yAxisEnabled_);
    publishAxisEnableState(
        otms::device::LogicalAxis::Z,
        zEnableStateAvailable,
        zEnabled != 0,
        zAxisEnableStateAvailable_,
        zAxisEnabled_);

    if (motionConnected_ != connected) {
        motionConnected_ = connected;
        emit motionConnectionChanged(motionConnected_);
    }
    if (motionReady_ != motionAvailable) {
        motionReady_ = motionAvailable;
        executor_.setMotionDeviceReady(databaseReady_ && motionReady_);
    }
    if (initializationState_ == otms::workflow::InitializationState::Completed
        && (!connected || !axesEnabled_)) {
        setInitializationState(
            otms::workflow::InitializationState::NotStarted,
            !connected
                ? QStringLiteral("运动控制器连接丢失，初始化结果已失效。")
                : QStringLiteral("存在未使能轴，初始化结果已失效。"));
    }
    if (positionAvailable) {
        emit motorPositionChanged(xPosition, yPosition, zPosition);
    }

    pollSafetyIo();
    updateStateFromConditions();
    pollInitializationMotion();
}

void MeasurementTaskController::pollSafetyIo()
{
    bool doorLocked = false;
    const bool doorLockStateAvailable = motionControllers_.readDoorLocked(doorLocked) == 1;
    if (doorLockStateAvailable_ != doorLockStateAvailable
        || (doorLockStateAvailable && doorLocked_ != doorLocked)) {
        doorLockStateAvailable_ = doorLockStateAvailable;
        doorLocked_ = doorLocked;
        emit doorLockStateChanged(doorLockStateAvailable_, doorLocked_);
    }

    bool lightCurtainClear = false;
    const bool lightCurtainStateAvailable =
        motionControllers_.readLightCurtainClear(lightCurtainClear) == 1;
    if (lightCurtainStateAvailable_ != lightCurtainStateAvailable
        || (lightCurtainStateAvailable && lightCurtainClear_ != lightCurtainClear)) {
        lightCurtainStateAvailable_ = lightCurtainStateAvailable;
        lightCurtainClear_ = lightCurtainClear;
        emit lightCurtainStateChanged(lightCurtainStateAvailable_, lightCurtainClear_);
    }
}

void MeasurementTaskController::pollInitializationMotion()
{
    if (initializationState_ != otms::workflow::InitializationState::Running
        || initializationMotionAxisMask_ == 0U) {
        return;
    }

    const auto axisArrived =
        [this](otms::device::LogicalAxis axis, unsigned int mask) {
        if ((initializationMotionAxisMask_ & mask) == 0U) {
            return true;
        }
        int status = 0;
        if (motionControllers_.getMotionStatus(axis, status) != 1) {
            const QString reason = QStringLiteral("初始化过程中读取回零状态失败。");
            abortAllAxes();
            failInitialization(reason);
            return false;
        }
        return status != 0;
    };

    const bool xArrived = axisArrived(otms::device::LogicalAxis::X, XAxisMask);
    if (machineState_ == otms::workflow::MachineState::Fault
        || initializationState_ == otms::workflow::InitializationState::Failed) {
        return;
    }
    const bool yArrived = axisArrived(otms::device::LogicalAxis::Y, YAxisMask);
    if (machineState_ == otms::workflow::MachineState::Fault
        || initializationState_ == otms::workflow::InitializationState::Failed) {
        return;
    }
    const bool zArrived = axisArrived(otms::device::LogicalAxis::Z, ZAxisMask);
    if (xArrived && yArrived && zArrived) {
        initializationMotionAxisMask_ = 0U;
        setInitializationState(
            otms::workflow::InitializationState::Completed,
            QStringLiteral("全轴使能和 XYZ 回零已完成。"));
        updateStateFromConditions();
    }
}

bool MeasurementTaskController::safetyConditionsMet() const
{
    return doorLockStateAvailable_
        && doorLocked_
        && lightCurtainStateAvailable_
        && lightCurtainClear_;
}

bool MeasurementTaskController::startupConditionsMet() const
{
    return initializationState_ == otms::workflow::InitializationState::Completed
        && databaseReady_
        && motionReady_
        && probeReady_
        && safetyConditionsMet();
}

QString MeasurementTaskController::startupConditionFailureReason() const
{
    QStringList failures;
    if (initializationState_ != otms::workflow::InitializationState::Completed) {
        failures.append(QStringLiteral("初始化未完成"));
    }
    if (!databaseReady_) {
        failures.append(QStringLiteral("测量数据库未就绪"));
    }
    if (!motionConnected_) {
        failures.append(QStringLiteral("运动控制器未连接"));
    } else if (!axesEnabled_) {
        failures.append(QStringLiteral("运动轴未全部使能"));
    } else if (!motionReady_) {
        failures.append(QStringLiteral("运动设备未就绪"));
    }
    if (!probeReady_) {
        failures.append(QStringLiteral("激光探头未就绪"));
    }
    if (!doorLockStateAvailable_) {
        failures.append(QStringLiteral("门锁状态不可用"));
    } else if (!doorLocked_) {
        failures.append(QStringLiteral("门锁已打开"));
    }
    if (!lightCurtainStateAvailable_) {
        failures.append(QStringLiteral("光幕状态不可用"));
    } else if (!lightCurtainClear_) {
        failures.append(QStringLiteral("光幕被遮挡"));
    }
    return failures.isEmpty()
        ? QStringLiteral("运行中安全条件或设备就绪条件异常。")
        : QStringLiteral("运行中%1。").arg(failures.join(QStringLiteral("、")));
}

QString MeasurementTaskController::safetyConditionFailureReason() const
{
    QStringList failures;
    if (!doorLockStateAvailable_) {
        failures.append(QStringLiteral("门锁状态不可用"));
    } else if (!doorLocked_) {
        failures.append(QStringLiteral("门锁已打开"));
    }
    if (!lightCurtainStateAvailable_) {
        failures.append(QStringLiteral("光幕状态不可用"));
    } else if (!lightCurtainClear_) {
        failures.append(QStringLiteral("光幕被遮挡"));
    }
    return failures.isEmpty()
        ? QStringLiteral("安全条件不满足。")
        : QStringLiteral("%1，无法执行操作。")
              .arg(failures.join(QStringLiteral("、")));
}

void MeasurementTaskController::updateStateFromConditions()
{
    if (workflowPolicy_.enforcesStateGuards()
        && initializationState_ == otms::workflow::InitializationState::Running
        && (!motionConnected_ || !axesEnabled_ || !safetyConditionsMet())) {
        abortAllAxes();
        QString reason;
        if (!motionConnected_) {
            reason = QStringLiteral("初始化过程中运动控制器连接丢失。");
        } else if (!axesEnabled_) {
            reason = QStringLiteral("初始化过程中存在未使能轴。");
        } else {
            reason = safetyConditionFailureReason();
        }
        failInitialization(reason);
        return;
    }

    const bool ready = startupConditionsMet();
    if (machineState_ == otms::workflow::MachineState::NotReady && ready) {
        setMachineState(otms::workflow::MachineState::Ready);
        return;
    }
    if (machineState_ == otms::workflow::MachineState::Ready && !ready) {
        setMachineState(otms::workflow::MachineState::NotReady);
        return;
    }
    if (workflowPolicy_.latchesOperationalFaults()
        && machineState_ == otms::workflow::MachineState::Running
        && !startupConditionsMet()) {
        enterMachineFault(startupConditionFailureReason());
    }
}

void MeasurementTaskController::finishRunning()
{
    if (machineState_ != otms::workflow::MachineState::Running) {
        return;
    }
    setMachineState(
        startupConditionsMet()
            ? otms::workflow::MachineState::Ready
            : otms::workflow::MachineState::NotReady);
}

void MeasurementTaskController::handleTaskFailure(const QString& reason)
{
    if (machineState_ == otms::workflow::MachineState::Fault) {
        return;
    }
    if (workflowPolicy_.latchesOperationalFaults()
        && (!motionConnected_ || !axesEnabled_ || !motionReady_ || !safetyConditionsMet())) {
        enterMachineFault(reason);
        return;
    }

    qCWarning(measurementWorkflowLog).noquote()
        << QStringLiteral("自动任务执行失败：%1").arg(reason);
    if (executor_.phase() == TaskExecutor::ExecutionPhase::Faulted) {
        executor_.resetFault();
    }
    finishRunning();
    emit errorOccurred(reason);
}

void MeasurementTaskController::failInitialization(const QString& reason)
{
    if (initializationState_ != otms::workflow::InitializationState::Running) {
        return;
    }
    qCWarning(measurementWorkflowLog).noquote()
        << QStringLiteral("初始化失败：%1").arg(reason);
    initializationMotionAxisMask_ = 0U;
    setInitializationState(otms::workflow::InitializationState::Failed, reason);
    setMachineState(otms::workflow::MachineState::NotReady);
    emit errorOccurred(reason);
}

void MeasurementTaskController::enterMachineFault(const QString& reason)
{
    abortAllAxes();
    arrivalPollTimer_.stop();
    initializationMotionAxisMask_ = 0U;
    if (machineState_ == otms::workflow::MachineState::Fault) {
        return;
    }
    qCCritical(measurementWorkflowLog).noquote()
        << QStringLiteral("机器进入故障：%1").arg(reason);
    if (initializationState_ == otms::workflow::InitializationState::Running) {
        setInitializationState(otms::workflow::InitializationState::Failed, reason);
    }
    setMachineState(otms::workflow::MachineState::Fault);
    executor_.failActiveTask(reason);
    emit errorOccurred(reason);
    emit machineFaultOccurred(reason);
}

void MeasurementTaskController::reportOperationFailure(const QString& detail)
{
    qCWarning(measurementWorkflowLog).noquote()
        << QStringLiteral("操作执行失败：%1").arg(detail);
    emit errorOccurred(detail);
}

void MeasurementTaskController::rejectOperation(const QString& detail)
{
    emit errorOccurred(detail);
    emit operationRejected(detail);
}

void MeasurementTaskController::setMachineState(otms::workflow::MachineState state)
{
    if (machineState_ == state) {
        return;
    }
    const otms::workflow::MachineState previousState = machineState_;
    machineState_ = state;
    qCInfo(measurementWorkflowLog)
        << "Machine state changed"
        << machineStateName(previousState) << "->" << machineStateName(machineState_);
    emit machineStateChanged(machineState_);
}

void MeasurementTaskController::setRunMode(otms::workflow::RunMode mode)
{
    if (runMode_ == mode) {
        return;
    }
    const otms::workflow::RunMode previousMode = runMode_;
    runMode_ = mode;
    qCInfo(measurementWorkflowLog)
        << "Run mode changed"
        << runModeName(previousMode) << "->" << runModeName(runMode_);
    emit runModeChanged(runMode_);
}

void MeasurementTaskController::setInitializationState(
    otms::workflow::InitializationState state,
    const QString& detail)
{
    if (initializationState_ == state && detail.isEmpty()) {
        return;
    }
    const otms::workflow::InitializationState previousState = initializationState_;
    initializationState_ = state;
    qCInfo(measurementWorkflowLog).noquote()
        << "Initialization state changed"
        << static_cast<int>(previousState) << "->" << static_cast<int>(initializationState_)
        << detail;
    emit initializationStateChanged(initializationState_, detail);
}

void MeasurementTaskController::abortAllAxes()
{
    motionControllers_.abortAxis(otms::device::LogicalAxis::X);
    motionControllers_.abortAxis(otms::device::LogicalAxis::Y);
    motionControllers_.abortAxis(otms::device::LogicalAxis::Z);
}

void MeasurementTaskController::prepareExperiment(
    const QString& taskRunId,
    quint64 executionId,
    const QString& taskType,
    int pointCount)
{
    preparedExperimentRunId_ = taskRunId;
    QString tableName;
    QString databaseError;
    experimentPrepared_ = database_.prepareExperiment(taskRunId, &tableName, &databaseError);
    if (!experimentPrepared_) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("创建实验数据表失败：runId=%1 executionId=%2 reason=%3")
                   .arg(logRunId(taskRunId))
                   .arg(executionId)
                   .arg(databaseError);
        emit errorOccurred(databaseError);
        return;
    }

    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral("实验数据表已创建：runId=%1 executionId=%2 type=%3 points=%4 table=%5")
               .arg(logRunId(taskRunId))
               .arg(executionId)
               .arg(taskType)
               .arg(pointCount)
               .arg(tableName);
}

void MeasurementTaskController::beginMove(
    const QString& taskRunId,
    quint64 executionId,
    int pointIndex,
    const TaskExecutionPoint& point)
{
    const bool experimentReady =
        experimentPrepared_ && preparedExperimentRunId_ == taskRunId;
    if (!databaseReady_ || !motionReady_ || !probeReady_ || !experimentReady) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral(
                   "点位执行条件未就绪：runId=%1 point=%2 database=%3 experiment=%4 motion=%5 probe=%6")
                   .arg(logRunId(taskRunId))
                   .arg(pointIndex + 1)
                   .arg(databaseReady_)
                   .arg(experimentReady)
                   .arg(motionReady_)
                   .arg(probeReady_);
        executor_.notifyMotionFailed(executionId, pointIndex, QStringLiteral("执行设备或数据库未就绪。"));
        return;
    }

    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral(
               "发送点位移动：runId=%1 point=%2 description=%3 物料=(%4,%5)mm 目标电机=(%6,%7)mm")
               .arg(logRunId(taskRunId))
               .arg(pointIndex + 1)
               .arg(point.pointDescription)
               .arg(point.workpiecePoint.x(), 0, 'f', 3)
               .arg(point.workpiecePoint.y(), 0, 'f', 3)
               .arg(point.motorTarget.x(), 0, 'f', 3)
               .arg(point.motorTarget.y(), 0, 'f', 3);

    const int xResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::X,
        point.motorTarget.x(),
        true);
    const int yResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::Y,
        point.motorTarget.y(),
        true);
    if (xResult != 1 || yResult != 1) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("移动命令失败：runId=%1 point=%2 xResult=%3 yResult=%4")
                   .arg(logRunId(taskRunId))
                   .arg(pointIndex + 1)
                   .arg(xResult)
                   .arg(yResult);
        motionControllers_.abortAxis(otms::device::LogicalAxis::X);
        motionControllers_.abortAxis(otms::device::LogicalAxis::Y);
        executor_.notifyMotionFailed(executionId, pointIndex, QStringLiteral("发送 X/Y 轴移动命令失败。"));
        return;
    }

    activeRunId_ = taskRunId;
    activeExecutionId_ = executionId;
    activePointIndex_ = pointIndex;
    activePoint_ = point;
    consecutiveArrivalSamples_ = 0;
    arrivalWait_.restart();
    arrivalPollTimer_.start();
    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral("开始等待到位：runId=%1 point=%2 timeout=%3ms")
               .arg(logRunId(taskRunId))
               .arg(pointIndex + 1)
               .arg(ArrivalTimeoutMs);
}

void MeasurementTaskController::pollArrival()
{
    double xPosition = 0.0;
    double yPosition = 0.0;
    int xStatus = 0;
    int yStatus = 0;
    if (motionControllers_.getPosition(otms::device::LogicalAxis::X, xPosition) != 1
        || motionControllers_.getPosition(otms::device::LogicalAxis::Y, yPosition) != 1
        || motionControllers_.getMotionStatus(otms::device::LogicalAxis::X, xStatus) != 1
        || motionControllers_.getMotionStatus(otms::device::LogicalAxis::Y, yStatus) != 1) {
        arrivalPollTimer_.stop();
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("到位状态读取失败：runId=%1 point=%2 elapsed=%3ms")
                   .arg(logRunId(activeRunId_))
                   .arg(activePointIndex_ + 1)
                   .arg(arrivalWait_.elapsed());
        executor_.notifyMotionFailed(
            activeExecutionId_, activePointIndex_, QStringLiteral("读取电机位置或到位状态失败。"));
        return;
    }

    const bool positionReached =
        std::abs(xPosition - activePoint_.motorTarget.x()) <= ArrivalToleranceMillimeters
        && std::abs(yPosition - activePoint_.motorTarget.y()) <= ArrivalToleranceMillimeters;
    if (xStatus != 0 && yStatus != 0 && positionReached) {
        ++consecutiveArrivalSamples_;
        if (consecutiveArrivalSamples_ >= RequiredArrivalSamples) {
            arrivalPollTimer_.stop();
            qCInfo(measurementWorkflowLog).noquote()
                << QStringLiteral(
                       "电机到位：runId=%1 point=%2 actual=(%3,%4)mm elapsed=%5ms samples=%6")
                       .arg(logRunId(activeRunId_))
                       .arg(activePointIndex_ + 1)
                       .arg(xPosition, 0, 'f', 3)
                       .arg(yPosition, 0, 'f', 3)
                       .arg(arrivalWait_.elapsed())
                       .arg(consecutiveArrivalSamples_);
            executor_.notifyMotionCompleted(
                activeExecutionId_, activePointIndex_, QPointF(xPosition, yPosition));
        }
        return;
    }

    consecutiveArrivalSamples_ = 0;
    if (arrivalWait_.elapsed() >= ArrivalTimeoutMs) {
        arrivalPollTimer_.stop();
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral(
                   "等待到位超时：runId=%1 point=%2 target=(%3,%4)mm actual=(%5,%6)mm elapsed=%7ms")
                   .arg(logRunId(activeRunId_))
                   .arg(activePointIndex_ + 1)
                   .arg(activePoint_.motorTarget.x(), 0, 'f', 3)
                   .arg(activePoint_.motorTarget.y(), 0, 'f', 3)
                   .arg(xPosition, 0, 'f', 3)
                   .arg(yPosition, 0, 'f', 3)
                   .arg(arrivalWait_.elapsed());
        motionControllers_.abortAxis(otms::device::LogicalAxis::X);
        motionControllers_.abortAxis(otms::device::LogicalAxis::Y);
        executor_.notifyMotionFailed(
            activeExecutionId_, activePointIndex_, QStringLiteral("等待电机到位超时（10 秒）。"));
    }
}

void MeasurementTaskController::measurePoint(
    const QString& taskRunId,
    quint64 executionId,
    int pointIndex,
    const TaskExecutionPoint& point,
    const QPointF& actualMotorPosition)
{
    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral("开始采集：runId=%1 point=%2 description=%3 motor=(%4,%5)mm")
               .arg(logRunId(taskRunId))
               .arg(pointIndex + 1)
               .arg(point.pointDescription)
               .arg(actualMotorPosition.x(), 0, 'f', 3)
               .arg(actualMotorPosition.y(), 0, 'f', 3);
    otms::device::LaserMeasurement measurement;
    const otms::device::LaserStatus status = devices_.measureLaserOnce(measurement);
    if (!status.ok()) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("激光采集失败：runId=%1 point=%2 code=%3 reason=%4")
                   .arg(logRunId(taskRunId))
                   .arg(pointIndex + 1)
                   .arg(status.vendorCode)
                   .arg(status.message);
        executor_.notifyMeasurementFailed(executionId, pointIndex, status.message);
        return;
    }
    if (measurement.quality != otms::device::MeasurementQuality::Valid
        || !std::isfinite(measurement.valueMicrometers)) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("激光采集值无效：runId=%1 point=%2 quality=%3 value=%4um")
                   .arg(logRunId(taskRunId))
                   .arg(pointIndex + 1)
                   .arg(static_cast<int>(measurement.quality))
                   .arg(measurement.valueMicrometers, 0, 'f', 3);
        executor_.notifyMeasurementFailed(executionId, pointIndex, QStringLiteral("激光测量值无效。"));
        return;
    }

    const QDateTime measuredAt = QDateTime::currentDateTimeUtc();
    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral(
               "采集完成：runId=%1 point=%2 description=%3 thickness=%4um quality=%5 trigger=%6 measuredAt=%7")
               .arg(logRunId(taskRunId))
               .arg(pointIndex + 1)
               .arg(point.pointDescription)
               .arg(measurement.valueMicrometers, 0, 'f', 3)
               .arg(static_cast<int>(measurement.quality))
               .arg(measurement.triggerCount)
               .arg(measuredAt.toString(Qt::ISODateWithMs));
    QString databaseError;
    if (!database_.insertMeasurement(
            taskRunId,
            pointIndex,
            point.pointDescription,
            actualMotorPosition,
            point.workpiecePoint,
            measurement.valueMicrometers,
            measuredAt,
            &databaseError)) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("测量记录入库失败：runId=%1 point=%2 reason=%3")
                   .arg(logRunId(taskRunId))
                   .arg(pointIndex + 1)
                   .arg(databaseError);
        executor_.notifyMeasurementFailed(executionId, pointIndex, databaseError);
        return;
    }

    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral("测量记录已入库：runId=%1 point=%2 description=%3")
               .arg(logRunId(taskRunId))
               .arg(pointIndex + 1)
               .arg(point.pointDescription);

    emit measurementAvailable(measurement.valueMicrometers / 1000.0);
    executor_.notifyMeasurementCompleted(
        executionId, pointIndex, measurement.valueMicrometers, measuredAt);
}

void MeasurementTaskController::stopMotion(quint64 executionId)
{
    qCWarning(measurementWorkflowLog)
        << "Stopping motion for execution" << executionId;
    arrivalPollTimer_.stop();
    initializationMotionAxisMask_ = 0U;
    abortAllAxes();
    executor_.notifyMotionStopped(executionId);
}

void MeasurementTaskController::stopProbe(quint64 executionId)
{
    qCInfo(measurementWorkflowLog)
        << "Probe stop acknowledged without continuous-measurement command for execution"
        << executionId;
    executor_.notifyProbeStopped(executionId);
}

void MeasurementTaskController::persistTaskLog(
    const QString& taskRunId,
    const QString& taskType,
    TaskExecutor::TaskResult result,
    const QString& detail,
    const QDateTime& finishedAt)
{
    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral("任务终态：runId=%1 类型=%2 结果=%3 detail=%4 finishedAt=%5")
               .arg(logRunId(taskRunId), taskType, taskResultName(result), detail)
               .arg(finishedAt.toString(Qt::ISODateWithMs));
    QString databaseError;
    if (!database_.insertTaskLog(
            taskRunId,
            taskType,
            taskResultName(result),
            detail,
            finishedAt,
            &databaseError)) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("任务终态日志入库失败：runId=%1 reason=%2")
                   .arg(logRunId(taskRunId), databaseError);
        emit errorOccurred(databaseError);
        return;
    }
    qCInfo(measurementWorkflowLog).noquote()
        << QStringLiteral("任务终态日志已入库：runId=%1").arg(logRunId(taskRunId));
    emit taskLogChanged();
}

void MeasurementTaskController::initializeDatabase()
{
    QString databaseError;
    databaseReady_ = database_.open(&databaseError);
    qCInfo(measurementWorkflowLog)
        << "Measurement database ready" << databaseReady_;
    if (!databaseReady_) {
        reportInitializationError(databaseError);
    }
}

void MeasurementTaskController::reportInitializationError(const QString& detail)
{
    QTimer::singleShot(0, this, [this, detail] {
        emit errorOccurred(detail);
    });
}
