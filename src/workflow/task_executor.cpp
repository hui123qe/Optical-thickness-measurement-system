#include "task_executor.h"

#include <limits>

#include <QStringList>
#include <QLoggingCategory>
#include <QUuid>

Q_LOGGING_CATEGORY(taskExecutorLog, "otms.workflow.executor")

namespace {

QString logRunId(const QString& runId)
{
    return runId.left(8);
}

} // namespace

TaskExecutor::TaskExecutor(QObject* parent)
    : QObject(parent)
{
}

TaskExecutor::ExecutionPhase TaskExecutor::phase() const
{
    return phase_;
}

TaskExecutor::TaskResult TaskExecutor::lastResult() const
{
    return lastResult_;
}

quint64 TaskExecutor::executionId() const
{
    return executionId_;
}

QString TaskExecutor::taskRunId() const
{
    return taskRunId_;
}

int TaskExecutor::currentPointIndex() const
{
    return currentPointIndex_;
}

QList<TaskPointResult> TaskExecutor::results() const
{
    return results_;
}

void TaskExecutor::setMotionDeviceReady(bool ready)
{
    if (motionDeviceReady_ == ready) {
        return;
    }

    motionDeviceReady_ = ready;
    emit readinessChanged(motionDeviceReady_, probeReady_);
    if (!motionDeviceReady_ && isTaskInProgress()) {
        enterFault(QStringLiteral("运动设备连接断开。"));
    }
}

void TaskExecutor::setProbeReady(bool ready)
{
    if (probeReady_ == ready) {
        return;
    }

    probeReady_ = ready;
    emit readinessChanged(motionDeviceReady_, probeReady_);
    if (!probeReady_ && isTaskInProgress()) {
        enterFault(QStringLiteral("激光探头连接断开。"));
    }
}

void TaskExecutor::start(const QString& taskType, const QList<TaskExecutionPoint>& points)
{
    qCInfo(taskExecutorLog).noquote()
        << QStringLiteral("收到开始请求：type=%1 points=%2 phase=%3 motionReady=%4 probeReady=%5")
               .arg(taskType)
               .arg(points.size())
               .arg(static_cast<int>(phase_))
               .arg(motionDeviceReady_)
               .arg(probeReady_);
    if (phase_ != ExecutionPhase::Idle) {
        qCWarning(taskExecutorLog) << "Start rejected: executor is not idle";
        emit executionRejected(QStringLiteral("执行器当前不是空闲状态。"));
        return;
    }
    if (points.isEmpty()) {
        qCWarning(taskExecutorLog) << "Start rejected: empty point queue";
        emit executionRejected(QStringLiteral("任务点队列为空。"));
        return;
    }
    if (points.size() > std::numeric_limits<int>::max()) {
        qCWarning(taskExecutorLog) << "Start rejected: point count out of range" << points.size();
        emit executionRejected(QStringLiteral("任务点数量超过执行器支持范围。"));
        return;
    }
    if (!motionDeviceReady_ || !probeReady_) {
        QStringList unavailableDevices;
        if (!motionDeviceReady_) {
            unavailableDevices.append(QStringLiteral("运动设备"));
        }
        if (!probeReady_) {
            unavailableDevices.append(QStringLiteral("激光探头"));
        }
        qCWarning(taskExecutorLog)
            << "Start rejected: devices not ready"
            << "motion" << motionDeviceReady_ << "probe" << probeReady_;
        emit executionRejected(
            QStringLiteral("无法开始任务：%1未就绪。")
                .arg(unavailableDevices.join(QStringLiteral("、"))));
        return;
    }

    ++executionId_;
    taskRunId_ = QUuid::createUuid().toString(QUuid::WithoutBraces);
    taskType_ = taskType.trimmed().isEmpty() ? QStringLiteral("未知任务") : taskType.trimmed();
    points_ = points;
    results_.clear();
    results_.reserve(points_.size());
    currentPointIndex_ = 0;
    qCInfo(taskExecutorLog).noquote()
        << QStringLiteral("开始请求已接受：runId=%1 executionId=%2 type=%3 points=%4")
               .arg(logRunId(taskRunId_))
               .arg(executionId_)
               .arg(taskType_)
               .arg(points_.size());
    setLastResult(TaskResult::None, QStringLiteral("新任务已开始。"));
    emit executionStarted(
        taskRunId_,
        executionId_,
        taskType_,
        static_cast<int>(points_.size()));
    requestCurrentPointMove();
}

void TaskExecutor::terminate()
{
    qCWarning(taskExecutorLog)
        << "Termination requested" << "runId" << logRunId(taskRunId_)
        << "phase" << static_cast<int>(phase_);
    if (phase_ == ExecutionPhase::Stopping) {
        emit executionRejected(QStringLiteral("任务正在停止。"));
        return;
    }
    if (phase_ != ExecutionPhase::Moving
        && phase_ != ExecutionPhase::Measuring) {
        emit executionRejected(QStringLiteral("当前没有正在执行的任务。"));
        return;
    }

    beginStopping(TaskResult::Terminated, QStringLiteral("用户终止任务。"));
}

void TaskExecutor::failActiveTask(const QString& reason)
{
    if (phase_ != ExecutionPhase::Moving
        && phase_ != ExecutionPhase::Measuring
        && phase_ != ExecutionPhase::Stopping) {
        return;
    }

    const QString detail = reason.trimmed().isEmpty()
        ? QStringLiteral("机器故障导致任务结束。")
        : reason;
    qCCritical(taskExecutorLog).noquote()
        << QStringLiteral("机器故障结束活动任务：runId=%1 phase=%2 reason=%3")
               .arg(logRunId(taskRunId_))
               .arg(static_cast<int>(phase_))
               .arg(detail);

    if (phase_ == ExecutionPhase::Moving
        || phase_ == ExecutionPhase::Measuring) {
        beginStopping(TaskResult::Failed, detail);
        return;
    }
    if (phase_ == ExecutionPhase::Stopping) {
        pendingStopResult_ = TaskResult::Failed;
        pendingStopDetail_ = detail;
    }
}

void TaskExecutor::resetFault()
{
    if (phase_ != ExecutionPhase::Faulted) {
        emit executionRejected(QStringLiteral("执行器当前不处于故障状态。"));
        return;
    }
    setPhase(ExecutionPhase::Idle, QStringLiteral("软件故障已复位。"));
}

void TaskExecutor::notifyMotionCompleted(
    quint64 executionId,
    int pointIndex,
    const QPointF& actualMotorPosition)
{
    if (phase_ != ExecutionPhase::Moving || !matchesCurrentPoint(executionId, pointIndex)) {
        return;
    }

    currentActualMotorPosition_ = actualMotorPosition;
    setPhase(
        ExecutionPhase::Measuring,
        QStringLiteral("第 %1/%2 个目标已到位，正在采集厚度。")
            .arg(currentPointIndex_ + 1)
            .arg(points_.size()));
    emit measurementRequested(
        taskRunId_,
        executionId_,
        currentPointIndex_,
        points_.at(currentPointIndex_),
        currentActualMotorPosition_);
}

void TaskExecutor::notifyMotionFailed(
    quint64 executionId,
    int pointIndex,
    const QString& reason)
{
    if (phase_ != ExecutionPhase::Moving || !matchesCurrentPoint(executionId, pointIndex)) {
        return;
    }
    beginStopping(
        TaskResult::Failed,
        QStringLiteral("第 %1 个目标运动失败：%2").arg(pointIndex + 1).arg(reason));
}

void TaskExecutor::notifyMotionStopped(quint64 executionId)
{
    if (phase_ != ExecutionPhase::Stopping || executionId != executionId_) {
        return;
    }
    motionStopped_ = true;
    finishStoppingIfReady();
}

void TaskExecutor::notifyMeasurementCompleted(
    quint64 executionId,
    int pointIndex,
    double measurement,
    const QDateTime& measuredAt)
{
    if (phase_ != ExecutionPhase::Measuring || !matchesCurrentPoint(executionId, pointIndex)) {
        return;
    }

    const TaskExecutionPoint point = points_.at(currentPointIndex_);
    results_.append(TaskPointResult{
        point,
        currentActualMotorPosition_,
        measurement,
        measuredAt});
    emit pointCompleted(
        taskRunId_,
        executionId_,
        currentPointIndex_,
        point,
        currentActualMotorPosition_,
        measurement,
        measuredAt);

    ++currentPointIndex_;
    if (currentPointIndex_ >= points_.size()) {
        const int pointCount = static_cast<int>(points_.size());
        const QString detail = QStringLiteral("任务完成，共测量 %1 个点。").arg(pointCount);
        setPhase(ExecutionPhase::Idle, QStringLiteral("当前无执行中的任务。"));
        finishTask(TaskResult::Completed, detail);
        emit executionCompleted(executionId_, pointCount);
        return;
    }
    requestCurrentPointMove();
}

void TaskExecutor::notifyMeasurementFailed(
    quint64 executionId,
    int pointIndex,
    const QString& reason)
{
    if (phase_ != ExecutionPhase::Measuring || !matchesCurrentPoint(executionId, pointIndex)) {
        return;
    }
    beginStopping(
        TaskResult::Failed,
        QStringLiteral("第 %1 个目标测量失败：%2").arg(pointIndex + 1).arg(reason));
}

void TaskExecutor::notifyProbeStopped(quint64 executionId)
{
    if (phase_ != ExecutionPhase::Stopping || executionId != executionId_) {
        return;
    }
    probeStopped_ = true;
    finishStoppingIfReady();
}

bool TaskExecutor::isTaskInProgress() const
{
    return phase_ == ExecutionPhase::Moving
        || phase_ == ExecutionPhase::Measuring
        || phase_ == ExecutionPhase::Stopping;
}

bool TaskExecutor::matchesCurrentPoint(quint64 executionId, int pointIndex) const
{
    return executionId == executionId_ && pointIndex == currentPointIndex_;
}

void TaskExecutor::requestCurrentPointMove()
{
    const TaskExecutionPoint point = points_.at(currentPointIndex_);
    setPhase(
        ExecutionPhase::Moving,
        QStringLiteral("正在移动到第 %1/%2 个目标（%3）：X=%4 mm，Y=%5 mm。")
            .arg(currentPointIndex_ + 1)
            .arg(points_.size())
            .arg(point.pointDescription)
            .arg(point.motorTarget.x(), 0, 'f', 3)
            .arg(point.motorTarget.y(), 0, 'f', 3));
    emit moveRequested(
        taskRunId_,
        executionId_,
        currentPointIndex_,
        point);
}

void TaskExecutor::beginStopping(TaskResult result, const QString& detail)
{
    pendingStopResult_ = result;
    pendingStopDetail_ = detail;
    motionStopped_ = false;
    probeStopped_ = false;
    setPhase(
        ExecutionPhase::Stopping,
        QStringLiteral("正在停止运动设备和激光探头：%1").arg(detail));
    emit motionStopRequested(executionId_);
    emit probeStopRequested(executionId_);
}

void TaskExecutor::finishStoppingIfReady()
{
    if (!motionStopped_ || !probeStopped_) {
        return;
    }

    const TaskResult result = pendingStopResult_;
    const QString detail = pendingStopDetail_;
    pendingStopResult_ = TaskResult::None;
    pendingStopDetail_.clear();
    setPhase(ExecutionPhase::Idle, QStringLiteral("运动设备和激光探头已停止。"));
    finishTask(result, detail);
    if (result == TaskResult::Terminated) {
        emit executionTerminated(executionId_);
        return;
    }
    emit executionFailed(executionId_, detail);
}

void TaskExecutor::enterFault(const QString& reason)
{
    emit motionStopRequested(executionId_);
    emit probeStopRequested(executionId_);
    pendingStopResult_ = TaskResult::None;
    pendingStopDetail_.clear();
    setPhase(ExecutionPhase::Faulted, reason);
    finishTask(TaskResult::Failed, reason);
    emit executionFailed(executionId_, reason);
}

void TaskExecutor::finishTask(TaskResult result, const QString& detail)
{
    setLastResult(result, detail);
    emit taskFinished(
        taskRunId_,
        taskType_,
        result,
        detail,
        QDateTime::currentDateTimeUtc());
}

void TaskExecutor::setPhase(ExecutionPhase phase, const QString& detail)
{
    const ExecutionPhase previousPhase = phase_;
    phase_ = phase;
    qCInfo(taskExecutorLog).noquote()
        << QStringLiteral("执行阶段变化：runId=%1 %2 -> %3 detail=%4")
               .arg(logRunId(taskRunId_))
               .arg(static_cast<int>(previousPhase))
               .arg(static_cast<int>(phase_))
               .arg(detail);
    emit phaseChanged(phase_, detail);
}

void TaskExecutor::setLastResult(TaskResult result, const QString& detail)
{
    lastResult_ = result;
    qCInfo(taskExecutorLog).noquote()
        << QStringLiteral("任务结果更新：runId=%1 result=%2 detail=%3")
               .arg(logRunId(taskRunId_))
               .arg(static_cast<int>(lastResult_))
               .arg(detail);
    emit taskResultChanged(lastResult_, detail);
}
