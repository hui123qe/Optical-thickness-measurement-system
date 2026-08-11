#include "measurement_task_controller.h"

#include "../device/device_manager.h"
#include "../motor/axis_manager.h"
#include "../motor/motion_driver_factory.h"

#include <cmath>
#include <cstdint>

#include <QPointF>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(measurementWorkflowLog, "otms.workflow.measurement")

namespace {

constexpr int LogicalXAxis = 0;
constexpr int LogicalYAxis = 1;
constexpr int ControllerXAxis = 0;
constexpr int ControllerYAxis = 1;
constexpr double VirtualCountsPerMillimeter = 1000.0;
constexpr int ArrivalPollIntervalMs = 20;
constexpr int MotorStatusPollIntervalMs = 200;
constexpr qint64 ArrivalTimeoutMs = 10000;
constexpr double ArrivalToleranceMillimeters = 0.001;
constexpr int RequiredArrivalSamples = 2;

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

MeasurementTaskController::MeasurementTaskController(TaskExecutor& executor, QObject* parent)
    : QObject(parent)
    , executor_(executor)
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

    initializeVirtualDevices();
    motorStatusPollTimer_.start();
}

void MeasurementTaskController::pollMotorStatus()
{
    otms::device::AxisManager& axes = otms::device::AxisManager::instance();
    int xConnected = 0;
    int yConnected = 0;
    const bool connected = axes.isConnected(LogicalXAxis, xConnected) == 1
        && axes.isConnected(LogicalYAxis, yConnected) == 1
        && xConnected != 0
        && yConnected != 0;

    double xPosition = 0.0;
    double yPosition = 0.0;
    const bool positionAvailable = connected
        && axes.getPosition(LogicalXAxis, xPosition) == 1
        && axes.getPosition(LogicalYAxis, yPosition) == 1;
    const bool motionAvailable = connected && positionAvailable;

    if (motionReady_ != motionAvailable) {
        motionReady_ = motionAvailable;
        executor_.setMotionDeviceReady(databaseReady_ && motionReady_);
        emit motionConnectionChanged(motionReady_);
    }
    if (motionAvailable) {
        emit motorPositionChanged(xPosition, yPosition);
    }
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

    otms::device::AxisManager& axes = otms::device::AxisManager::instance();
    const int xResult = axes.moveAbsolute(LogicalXAxis, point.motorTarget.x(), true);
    const int yResult = axes.moveAbsolute(LogicalYAxis, point.motorTarget.y(), true);
    if (xResult != 1 || yResult != 1) {
        qCCritical(measurementWorkflowLog).noquote()
            << QStringLiteral("移动命令失败：runId=%1 point=%2 xResult=%3 yResult=%4")
                   .arg(logRunId(taskRunId))
                   .arg(pointIndex + 1)
                   .arg(xResult)
                   .arg(yResult);
        axes.abort(LogicalXAxis);
        axes.abort(LogicalYAxis);
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
    otms::device::AxisManager& axes = otms::device::AxisManager::instance();
    double xPosition = 0.0;
    double yPosition = 0.0;
    int xStatus = 0;
    int yStatus = 0;
    if (axes.getPosition(LogicalXAxis, xPosition) != 1
        || axes.getPosition(LogicalYAxis, yPosition) != 1
        || axes.getMotionStatus(LogicalXAxis, xStatus) != 1
        || axes.getMotionStatus(LogicalYAxis, yStatus) != 1) {
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
        axes.abort(LogicalXAxis);
        axes.abort(LogicalYAxis);
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
    const otms::device::LaserStatus status =
        otms::device::DeviceManager::instance().measureLaserOnce(measurement);
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
    otms::device::AxisManager& axes = otms::device::AxisManager::instance();
    axes.abort(LogicalXAxis);
    axes.abort(LogicalYAxis);
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

void MeasurementTaskController::initializeVirtualDevices()
{
    QString databaseError;
    databaseReady_ = database_.open(&databaseError);
    qCInfo(measurementWorkflowLog)
        << "Measurement database ready" << databaseReady_;
    if (!databaseReady_) {
        reportInitializationError(databaseError);
    }

    if (!otms::device::g_useVirtualMotionDriver) {
        reportInitializationError(QStringLiteral("真实运动控制器未配置，自动任务保持禁用。"));
    } else {
        motionDriver_ = otms::device::createMotionDriver(
            QString(), otms::device::AgtioControllerType::Agc301);
        otms::device::AxisManager& axes = otms::device::AxisManager::instance();
        const bool mapped = axes.setAxisMapping(
                                LogicalXAxis,
                                motionDriver_,
                                ControllerXAxis,
                                VirtualCountsPerMillimeter)
            && axes.setAxisMapping(
                LogicalYAxis,
                motionDriver_,
                ControllerYAxis,
                VirtualCountsPerMillimeter);
        motionReady_ = mapped
            && axes.connect(LogicalXAxis) == 1
            && axes.enable(LogicalXAxis) == 1
            && axes.enable(LogicalYAxis) == 1;
        qCInfo(measurementWorkflowLog)
            << "Virtual motion initialization" << "mapped" << mapped << "ready" << motionReady_;
        if (!motionReady_) {
            reportInitializationError(QStringLiteral("虚拟运动控制器连接或使能失败。"));
        }
    }

    if (!otms::device::g_useVirtualLaserProbe) {
        reportInitializationError(QStringLiteral("真实激光设备未配置，自动任务保持禁用。"));
    } else {
        otms::device::DeviceManager& devices = otms::device::DeviceManager::instance();
        std::uint8_t programNumber = 0;
        const otms::device::LaserStatus configurationStatus =
            devices.loadLaserConfiguration(programNumber);
        const otms::device::LaserStatus connectionStatus = configurationStatus.ok()
            ? devices.connectLaserEthernet()
            : configurationStatus;
        const otms::device::LaserStatus enableStatus = connectionStatus.ok()
            ? devices.enableLaser()
            : connectionStatus;
        probeReady_ = enableStatus.ok();
        qCInfo(measurementWorkflowLog)
            << "Virtual laser initialization" << "ready" << probeReady_
            << "program" << programNumber << "status" << enableStatus.vendorCode;
        if (!probeReady_) {
            reportInitializationError(QStringLiteral("虚拟激光探头初始化失败：%1").arg(enableStatus.message));
        }
    }

    executor_.setMotionDeviceReady(databaseReady_ && motionReady_);
    executor_.setProbeReady(databaseReady_ && probeReady_);
    QTimer::singleShot(0, this, [this] {
        emit motionConnectionChanged(motionReady_);
    });
}

void MeasurementTaskController::reportInitializationError(const QString& detail)
{
    QTimer::singleShot(0, this, [this, detail] {
        emit errorOccurred(detail);
    });
}
