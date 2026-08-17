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

unsigned int axisMask(otms::device::LogicalAxis axis)
{
    switch (axis) {
    case otms::device::LogicalAxis::X:
        return XAxisMask;
    case otms::device::LogicalAxis::Y:
        return YAxisMask;
    case otms::device::LogicalAxis::Z:
        return ZAxisMask;
    }
    return 0U;
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
    TaskExecutor& executor,
    otms::device::MotionControllerManager& motionControllers,
    otms::device::DeviceManager& devices,
    QObject* parent)
    : QObject(parent)
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
            manualAxisMask_ = 0U;
            setMachineState(
                otms::workflow::MachineState::Running,
                otms::workflow::RunMode::Automatic);
        });
    connect(&executor_, &TaskExecutor::executionCompleted,
        this, [this](quint64, int) { finishRunning(); });
    connect(&executor_, &TaskExecutor::executionTerminated,
        this, [this](quint64) { finishRunning(); });
    connect(&executor_, &TaskExecutor::executionFailed,
        this, [this](quint64, const QString& reason) { enterMachineFault(reason); });
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

bool MeasurementTaskController::moveAxisAbsolute(
    otms::device::LogicalAxis axis,
    double position)
{
    if (machineState_ != otms::workflow::MachineState::Ready) {
        emit errorOccurred(QStringLiteral("机器未就绪，无法执行手动运动。"));
        return false;
    }
    if (motionControllers_.moveAbsolute(axis, position, true) != 1) {
        enterMachineFault(QStringLiteral("手动绝对定位命令失败。"));
        return false;
    }

    manualAxisMask_ = axisMask(axis);
    setMachineState(
        otms::workflow::MachineState::Running,
        otms::workflow::RunMode::Manual);
    return true;
}

bool MeasurementTaskController::moveAxisRelative(
    otms::device::LogicalAxis axis,
    double distance)
{
    if (machineState_ != otms::workflow::MachineState::Ready) {
        emit errorOccurred(QStringLiteral("机器未就绪，无法执行手动运动。"));
        return false;
    }
    if (motionControllers_.moveRelative(axis, distance, true) != 1) {
        enterMachineFault(QStringLiteral("手动相对移动命令失败。"));
        return false;
    }

    manualAxisMask_ = axisMask(axis);
    setMachineState(
        otms::workflow::MachineState::Running,
        otms::workflow::RunMode::Manual);
    return true;
}

bool MeasurementTaskController::moveStageAbsolute(
    double xPosition,
    double yPosition,
    double zPosition)
{
    if (machineState_ != otms::workflow::MachineState::Ready) {
        emit errorOccurred(QStringLiteral("机器未就绪，无法执行手动运动。"));
        return false;
    }

    const int xResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::X, xPosition, true);
    const int yResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::Y, yPosition, true);
    const int zResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::Z, zPosition, true);
    if (xResult != 1 || yResult != 1 || zResult != 1) {
        enterMachineFault(QStringLiteral("XYZ 联动绝对定位命令失败。"));
        return false;
    }

    manualAxisMask_ = XAxisMask | YAxisMask | ZAxisMask;
    setMachineState(
        otms::workflow::MachineState::Running,
        otms::workflow::RunMode::Manual);
    return true;
}

bool MeasurementTaskController::moveWorkpiecePoint(double xPosition, double yPosition)
{
    if (machineState_ != otms::workflow::MachineState::Ready) {
        emit errorOccurred(QStringLiteral("机器未就绪，无法执行手动运动。"));
        return false;
    }

    const int xResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::X, xPosition, true);
    const int yResult = motionControllers_.moveAbsolute(
        otms::device::LogicalAxis::Y, yPosition, true);
    if (xResult != 1 || yResult != 1) {
        enterMachineFault(QStringLiteral("物料测点手动定位命令失败。"));
        return false;
    }

    manualAxisMask_ = XAxisMask | YAxisMask;
    setMachineState(
        otms::workflow::MachineState::Running,
        otms::workflow::RunMode::Manual);
    return true;
}

void MeasurementTaskController::startAutomaticTask(
    const QString& taskType,
    const QList<TaskExecutionPoint>& points)
{
    if (machineState_ != otms::workflow::MachineState::Ready) {
        emit errorOccurred(QStringLiteral("机器未就绪，无法启动自动实验。"));
        return;
    }
    executor_.start(taskType, points);
}

void MeasurementTaskController::emergencyStop()
{
    qCCritical(measurementWorkflowLog) << "Machine emergency stop requested";
    if (executor_.phase() == TaskExecutor::ExecutionPhase::Moving
        || executor_.phase() == TaskExecutor::ExecutionPhase::Measuring) {
        executor_.terminate();
    }
    if (machineState_ == otms::workflow::MachineState::Fault) {
        abortAllAxes();
        return;
    }
    enterMachineFault(QStringLiteral("软件急停已触发。"));
}

bool MeasurementTaskController::resetMachineFault()
{
    if (machineState_ != otms::workflow::MachineState::Fault) {
        emit errorOccurred(QStringLiteral("机器当前不处于故障状态，无需复位。"));
        return false;
    }
    if (executor_.phase() == TaskExecutor::ExecutionPhase::Stopping) {
        emit errorOccurred(QStringLiteral("设备仍在停止过程中，请稍后复位。"));
        return false;
    }
    if (executor_.phase() == TaskExecutor::ExecutionPhase::Faulted) {
        executor_.resetFault();
    }

    manualAxisMask_ = 0U;
    setMachineState(
        startupConditionsMet()
            ? otms::workflow::MachineState::Ready
            : otms::workflow::MachineState::NotReady,
        runMode_);
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
    const bool motionAvailable = connected && positionAvailable;

    if (motionReady_ != motionAvailable) {
        motionReady_ = motionAvailable;
        executor_.setMotionDeviceReady(databaseReady_ && motionReady_);
        emit motionConnectionChanged(motionReady_);
    }
    if (motionAvailable) {
        emit motorPositionChanged(xPosition, yPosition, zPosition);
    }

    pollSafetyIo();
    updateStateFromConditions();
    pollManualMotion();
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

void MeasurementTaskController::pollManualMotion()
{
    if (machineState_ != otms::workflow::MachineState::Running
        || runMode_ != otms::workflow::RunMode::Manual
        || manualAxisMask_ == 0U) {
        return;
    }

    const auto axisArrived = [this](otms::device::LogicalAxis axis, unsigned int mask) {
        if ((manualAxisMask_ & mask) == 0U) {
            return true;
        }
        int status = 0;
        if (motionControllers_.getMotionStatus(axis, status) != 1) {
            enterMachineFault(QStringLiteral("读取手动运动到位状态失败。"));
            return false;
        }
        return status != 0;
    };

    const bool xArrived = axisArrived(otms::device::LogicalAxis::X, XAxisMask);
    if (machineState_ == otms::workflow::MachineState::Fault) {
        return;
    }
    const bool yArrived = axisArrived(otms::device::LogicalAxis::Y, YAxisMask);
    if (machineState_ == otms::workflow::MachineState::Fault) {
        return;
    }
    const bool zArrived = axisArrived(otms::device::LogicalAxis::Z, ZAxisMask);
    if (xArrived && yArrived && zArrived) {
        manualAxisMask_ = 0U;
        finishRunning();
    }
}

bool MeasurementTaskController::startupConditionsMet() const
{
    return databaseReady_
        && motionReady_
        && probeReady_
        && doorLockStateAvailable_
        && doorLocked_
        && lightCurtainStateAvailable_
        && lightCurtainClear_;
}

void MeasurementTaskController::updateStateFromConditions()
{
    const bool ready = startupConditionsMet();
    if (machineState_ == otms::workflow::MachineState::NotReady && ready) {
        setMachineState(otms::workflow::MachineState::Ready, runMode_);
        return;
    }
    if (machineState_ == otms::workflow::MachineState::Ready && !ready) {
        setMachineState(otms::workflow::MachineState::NotReady, runMode_);
        return;
    }
    if (machineState_ == otms::workflow::MachineState::Running && !ready) {
        enterMachineFault(QStringLiteral("运行中安全条件或设备就绪条件异常。"));
    }
}

void MeasurementTaskController::finishRunning()
{
    if (machineState_ != otms::workflow::MachineState::Running) {
        return;
    }
    manualAxisMask_ = 0U;
    setMachineState(
        startupConditionsMet()
            ? otms::workflow::MachineState::Ready
            : otms::workflow::MachineState::NotReady,
        runMode_);
}

void MeasurementTaskController::enterMachineFault(const QString& reason)
{
    if (machineState_ == otms::workflow::MachineState::Fault) {
        return;
    }
    qCCritical(measurementWorkflowLog).noquote()
        << QStringLiteral("机器进入故障：%1").arg(reason);
    abortAllAxes();
    manualAxisMask_ = 0U;
    setMachineState(otms::workflow::MachineState::Fault, runMode_);
    emit errorOccurred(reason);
}

void MeasurementTaskController::setMachineState(
    otms::workflow::MachineState state,
    otms::workflow::RunMode mode)
{
    if (machineState_ == state && runMode_ == mode) {
        return;
    }
    const otms::workflow::MachineState previousState = machineState_;
    const otms::workflow::RunMode previousMode = runMode_;
    machineState_ = state;
    runMode_ = mode;
    qCInfo(measurementWorkflowLog)
        << "Machine state changed"
        << static_cast<int>(previousState) << "->" << static_cast<int>(machineState_)
        << "mode" << static_cast<int>(previousMode) << "->" << static_cast<int>(runMode_);
    emit machineStateChanged(machineState_, runMode_);
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
    motionControllers_.abortAxis(otms::device::LogicalAxis::X);
    motionControllers_.abortAxis(otms::device::LogicalAxis::Y);
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
