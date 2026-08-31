#pragma once

#include "machine_state.h"

#include <QString>
#include <QStringList>

namespace otms::workflow {

enum class OperationProfile
{
    Production,
    DebugBypass
};

enum class OperationKind
{
    ManualMotion,
    ManualAxisEnable,
    AutomaticTask
};

struct WorkflowStateSnapshot
{
    MachineState machineState{MachineState::NotReady};
    RunMode runMode{RunMode::Manual};
    bool automaticTaskInProgress{};
    bool motionConnected{};
    bool safetyConditionsMet{};
    bool emergencyStopLatched{};
    QString safetyFailureReason;
};

struct OperationDecision
{
    bool allowed{};
    bool bypassed{};
    QString rejectionReason;
    QStringList bypassedConditions;
};

class WorkflowPolicy final
{
public:
    [[nodiscard]] OperationProfile profile() const;
    void setProfile(OperationProfile profile);

    [[nodiscard]] bool enforcesStateGuards() const;
    [[nodiscard]] bool latchesOperationalFaults() const;
    [[nodiscard]] OperationDecision evaluate(
        OperationKind operation,
        const WorkflowStateSnapshot& state) const;

private:
    OperationProfile profile_{OperationProfile::Production};
};

} // namespace otms::workflow
