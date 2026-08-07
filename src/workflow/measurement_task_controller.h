#pragma once

#include "../data/measurement_database.h"
#include "task_executor.h"

#include <memory>

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace otms::device {
class IMotionDriver;
}

class MeasurementTaskController final : public QObject
{
    Q_OBJECT

public:
    explicit MeasurementTaskController(TaskExecutor& executor, QObject* parent = nullptr);

signals:
    void motionConnectionChanged(bool connected);
    void measurementAvailable(double measurementMillimeters);
    void taskLogChanged();
    void errorOccurred(const QString& detail);

private slots:
    void prepareExperiment(
        const QString& taskRunId,
        quint64 executionId,
        const QString& taskType,
        int pointCount);
    void beginMove(
        const QString& taskRunId,
        quint64 executionId,
        int pointIndex,
        const TaskExecutionPoint& point);
    void pollArrival();
    void measurePoint(
        const QString& taskRunId,
        quint64 executionId,
        int pointIndex,
        const TaskExecutionPoint& point,
        const QPointF& actualMotorPosition);
    void stopMotion(quint64 executionId);
    void stopProbe(quint64 executionId);
    void persistTaskLog(
        const QString& taskRunId,
        const QString& taskType,
        TaskExecutor::TaskResult result,
        const QString& detail,
        const QDateTime& finishedAt);

private:
    void initializeVirtualDevices();
    void reportInitializationError(const QString& detail);

    TaskExecutor& executor_;
    std::shared_ptr<otms::device::IMotionDriver> motionDriver_;
    MeasurementDatabase database_;
    QTimer arrivalPollTimer_;
    QElapsedTimer arrivalWait_;
    TaskExecutionPoint activePoint_;
    QString activeRunId_;
    quint64 activeExecutionId_{};
    int activePointIndex_{-1};
    int consecutiveArrivalSamples_{};
    QString preparedExperimentRunId_;
    bool experimentPrepared_{};
    bool databaseReady_{};
    bool motionReady_{};
    bool probeReady_{};
};
