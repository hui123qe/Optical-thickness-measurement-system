#pragma once

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPointF>
#include <QString>

struct TaskExecutionPoint
{
    QPointF workpiecePoint;
    QPointF motorTarget;
    QString pointDescription;
};

struct TaskPointResult
{
    TaskExecutionPoint point;
    QPointF actualMotorPosition;
    double measurement{};
    QDateTime measuredAt;
};

class TaskExecutor final : public QObject
{
    Q_OBJECT

public:
    enum class ExecutionPhase
    {
        Idle,
        Moving,
        Measuring,
        Stopping,
        Faulted
    };
    Q_ENUM(ExecutionPhase)

    enum class TaskResult
    {
        None,
        Completed,
        Terminated,
        Failed
    };
    Q_ENUM(TaskResult)

    explicit TaskExecutor(QObject* parent = nullptr);

    [[nodiscard]] ExecutionPhase phase() const;
    [[nodiscard]] TaskResult lastResult() const;
    [[nodiscard]] quint64 executionId() const;
    [[nodiscard]] QString taskRunId() const;
    [[nodiscard]] int currentPointIndex() const;
    [[nodiscard]] QList<TaskPointResult> results() const;

public slots:
    void setMotionDeviceReady(bool ready);
    void setProbeReady(bool ready);
    void start(const QString& taskType, const QList<TaskExecutionPoint>& points);
    void terminate();
    void failActiveTask(const QString& reason);
    void resetFault();

    void notifyMotionCompleted(
        quint64 executionId,
        int pointIndex,
        const QPointF& actualMotorPosition);
    void notifyMotionFailed(quint64 executionId, int pointIndex, const QString& reason);
    void notifyMotionStopped(quint64 executionId);
    void notifyMeasurementCompleted(
        quint64 executionId,
        int pointIndex,
        double measurement,
        const QDateTime& measuredAt);
    void notifyMeasurementFailed(quint64 executionId, int pointIndex, const QString& reason);
    void notifyProbeStopped(quint64 executionId);

signals:
    void phaseChanged(TaskExecutor::ExecutionPhase phase, const QString& detail);
    void taskResultChanged(TaskExecutor::TaskResult result, const QString& detail);
    void executionRejected(const QString& reason);
    void readinessChanged(bool motionDeviceReady, bool probeReady);

    void executionStarted(
        const QString& taskRunId,
        quint64 executionId,
        const QString& taskType,
        int pointCount);
    void moveRequested(
        const QString& taskRunId,
        quint64 executionId,
        int pointIndex,
        const TaskExecutionPoint& point);
    void measurementRequested(
        const QString& taskRunId,
        quint64 executionId,
        int pointIndex,
        const TaskExecutionPoint& point,
        const QPointF& actualMotorPosition);
    void motionStopRequested(quint64 executionId);
    void probeStopRequested(quint64 executionId);

    void pointCompleted(
        const QString& taskRunId,
        quint64 executionId,
        int pointIndex,
        const TaskExecutionPoint& point,
        const QPointF& actualMotorPosition,
        double measurement,
        const QDateTime& measuredAt);
    void executionCompleted(quint64 executionId, int pointCount);
    void executionTerminated(quint64 executionId);
    void executionFailed(quint64 executionId, const QString& reason);
    void taskFinished(
        const QString& taskRunId,
        const QString& taskType,
        TaskExecutor::TaskResult result,
        const QString& detail,
        const QDateTime& finishedAt);

private:
    [[nodiscard]] bool isTaskInProgress() const;
    [[nodiscard]] bool matchesCurrentPoint(quint64 executionId, int pointIndex) const;
    void requestCurrentPointMove();
    void beginStopping(TaskResult result, const QString& detail);
    void finishStoppingIfReady();
    void enterFault(const QString& reason);
    void finishTask(TaskResult result, const QString& detail);
    void setPhase(ExecutionPhase phase, const QString& detail);
    void setLastResult(TaskResult result, const QString& detail);

    ExecutionPhase phase_{ExecutionPhase::Idle};
    TaskResult lastResult_{TaskResult::None};
    TaskResult pendingStopResult_{TaskResult::None};
    quint64 executionId_{};
    QString taskRunId_;
    QString taskType_;
    int currentPointIndex_{-1};
    bool motionDeviceReady_{};
    bool probeReady_{};
    bool motionStopped_{};
    bool probeStopped_{};
    QString pendingStopDetail_;
    QPointF currentActualMotorPosition_;
    QList<TaskExecutionPoint> points_;
    QList<TaskPointResult> results_;
};

Q_DECLARE_METATYPE(TaskExecutionPoint)
Q_DECLARE_METATYPE(QList<TaskExecutionPoint>)
