#pragma once

#include "../data/measurement_database.h"
#include "../motor/motion_controller_manager.h"
#include "machine_state.h"
#include "task_executor.h"
#include "workflow_policy.h"

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
        otms::workflow::WorkflowPolicy& workflowPolicy,
        TaskExecutor& executor,
        otms::device::MotionControllerManager& motionControllers,
        otms::device::DeviceManager& devices,
        QObject* parent = nullptr);

    [[nodiscard]] otms::workflow::MachineState machineState() const;
    [[nodiscard]] otms::workflow::RunMode runMode() const;
    [[nodiscard]] otms::workflow::InitializationState initializationState() const;
    [[nodiscard]] otms::workflow::OperationProfile operationProfile() const;
    [[nodiscard]] QStringList motionControllerIds() const;
    [[nodiscard]] bool isMotionControllerConnected(const QString& controllerId) const;
    [[nodiscard]] bool isLaserConnected() const;

    bool setDoorLocked(bool locked);
    bool connectMotionController(const QString& controllerId);
    bool disconnectMotionController(const QString& controllerId);
    bool connectLaser();
    bool disconnectLaser();
    bool switchRunMode(otms::workflow::RunMode mode);
    bool switchOperationProfile(otms::workflow::OperationProfile profile);
    bool initializeMachine();
    bool homeAxis(otms::device::LogicalAxis axis);
    bool homeStage();
    bool setAxisEnabled(otms::device::LogicalAxis axis, bool enabled);
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
    void axisEnableStateChanged(
        otms::device::LogicalAxis axis,
        bool available,
        bool enabled);
    void measurementAvailable(double measurementMillimeters);
    void doorLockStateChanged(bool available, bool locked);
    void lightCurtainStateChanged(bool available, bool clear);
    void machineStateChanged(otms::workflow::MachineState state);
    void runModeChanged(otms::workflow::RunMode mode);
    void operationProfileChanged(otms::workflow::OperationProfile profile);
    void initializationStateChanged(
        otms::workflow::InitializationState state,
        const QString& detail);
    void taskLogChanged();
    void operationRejected(const QString& detail);
    void machineFaultOccurred(const QString& detail);
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
    void pollInitializationMotion();
    [[nodiscard]] otms::workflow::WorkflowStateSnapshot workflowStateSnapshot() const;
    [[nodiscard]] bool operationAllowed(otms::workflow::OperationKind operation);
    [[nodiscard]] bool safetyConditionsMet() const;
    [[nodiscard]] bool startupConditionsMet() const;
    [[nodiscard]] QString startupConditionFailureReason() const;
    [[nodiscard]] QString safetyConditionFailureReason() const;
    void updateStateFromConditions();
    void finishRunning();
    void handleTaskFailure(const QString& reason);
    void failInitialization(const QString& reason);
    void enterMachineFault(const QString& reason);
    void reportOperationFailure(const QString& detail);
    void rejectOperation(const QString& detail);
    void setMachineState(otms::workflow::MachineState state);
    void setRunMode(otms::workflow::RunMode mode);
    void setInitializationState(
        otms::workflow::InitializationState state,
        const QString& detail);
    void abortAllAxes();
    void reportInitializationError(const QString& detail);

    otms::workflow::WorkflowPolicy& workflowPolicy_;
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
    bool motionConnected_{};
    bool motionReady_{};
    bool axesEnabled_{};
    bool xAxisEnableStateAvailable_{};
    bool yAxisEnableStateAvailable_{};
    bool zAxisEnableStateAvailable_{};
    bool xAxisEnabled_{};
    bool yAxisEnabled_{};
    bool zAxisEnabled_{};
    bool probeReady_{};
    bool doorLockStateAvailable_{};
    bool doorLocked_{};
    bool lightCurtainStateAvailable_{};
    bool lightCurtainClear_{};
    otms::workflow::MachineState machineState_{otms::workflow::MachineState::NotReady};
    otms::workflow::RunMode runMode_{otms::workflow::RunMode::Manual};
    otms::workflow::InitializationState initializationState_{
        otms::workflow::InitializationState::NotStarted};
    unsigned int initializationMotionAxisMask_{};
};
