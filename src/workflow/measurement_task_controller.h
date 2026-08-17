#pragma once

#include "../data/measurement_database.h"
#include "../motor/motion_controller_manager.h"
#include "machine_state.h"
#include "task_executor.h"

#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QTimer>

namespace otms::device {
class DeviceManager;
}

class MeasurementTaskController final : public QObject
{
    Q_OBJECT

public:
    explicit MeasurementTaskController(
        TaskExecutor& executor,
        otms::device::MotionControllerManager& motionControllers,
        otms::device::DeviceManager& devices,
        QObject* parent = nullptr);

    [[nodiscard]] otms::workflow::MachineState machineState() const;
    [[nodiscard]] otms::workflow::RunMode runMode() const;
    [[nodiscard]] QStringList motionControllerIds() const;
    [[nodiscard]] bool isMotionControllerConnected(const QString& controllerId) const;
    [[nodiscard]] bool isLaserConnected() const;

    bool setDoorLocked(bool locked);
    bool connectMotionController(const QString& controllerId);
    bool disconnectMotionController(const QString& controllerId);
    bool connectLaser();
    bool disconnectLaser();
    bool moveAxisAbsolute(otms::device::LogicalAxis axis, double position);
    bool moveAxisRelative(otms::device::LogicalAxis axis, double distance);
    bool moveStageAbsolute(double xPosition, double yPosition, double zPosition);
    bool moveWorkpiecePoint(double xPosition, double yPosition);
    void startAutomaticTask(
        const QString& taskType,
        const QList<TaskExecutionPoint>& points);
    void emergencyStop();
    bool resetMachineFault();

signals:
    void motionConnectionChanged(bool connected);
    void motorPositionChanged(
        double xMillimeters,
        double yMillimeters,
        double zMillimeters);
    void measurementAvailable(double measurementMillimeters);
    void doorLockStateChanged(bool available, bool locked);
    void lightCurtainStateChanged(bool available, bool clear);
    void machineStateChanged(
        otms::workflow::MachineState state,
        otms::workflow::RunMode mode);
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
    void pollMotorStatus();
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
    void initializeDatabase();
    void pollSafetyIo();
    void pollManualMotion();
    [[nodiscard]] bool startupConditionsMet() const;
    void updateStateFromConditions();
    void finishRunning();
    void enterMachineFault(const QString& reason);
    void setMachineState(
        otms::workflow::MachineState state,
        otms::workflow::RunMode mode);
    void abortAllAxes();
    void reportInitializationError(const QString& detail);

    TaskExecutor& executor_;
    otms::device::MotionControllerManager& motionControllers_;
    otms::device::DeviceManager& devices_;
    MeasurementDatabase database_;
    QTimer arrivalPollTimer_;
    QTimer motorStatusPollTimer_;
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
    bool doorLockStateAvailable_{};
    bool doorLocked_{};
    bool lightCurtainStateAvailable_{};
    bool lightCurtainClear_{};
    otms::workflow::MachineState machineState_{otms::workflow::MachineState::NotReady};
    otms::workflow::RunMode runMode_{otms::workflow::RunMode::Manual};
    unsigned int manualAxisMask_{};
};
