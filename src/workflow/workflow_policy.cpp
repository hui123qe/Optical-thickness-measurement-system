#include "workflow_policy.h"

namespace otms::workflow {

namespace {

OperationDecision rejected(const QString& reason)
{
    return OperationDecision{false, false, reason, {}};
}

void appendBypassedCondition(
    QStringList& conditions,
    bool conditionFailed,
    const QString& description)
{
    if (conditionFailed) {
        conditions.append(description);
    }
}

} // namespace

OperationProfile WorkflowPolicy::profile() const
{
    return profile_;
}

void WorkflowPolicy::setProfile(OperationProfile profile)
{
    profile_ = profile;
}

bool WorkflowPolicy::enforcesStateGuards() const
{
    return profile_ == OperationProfile::Production;
}

bool WorkflowPolicy::latchesOperationalFaults() const
{
    return profile_ == OperationProfile::Production;
}

OperationDecision WorkflowPolicy::evaluate(
    OperationKind operation,
    const WorkflowStateSnapshot& state) const
{
    if (state.emergencyStopLatched) {
        return rejected(QStringLiteral("软件急停已锁存，请先完成故障复位。"));
    }

    if ((operation == OperationKind::ManualMotion
            || operation == OperationKind::ManualAxisEnable)
        && state.automaticTaskInProgress) {
        switch (operation) {
        case OperationKind::ManualMotion:
            return rejected(QStringLiteral("自动任务正在运行，无法执行手动运动。"));
        case OperationKind::ManualAxisEnable:
            return rejected(QStringLiteral("自动任务正在运行，无法改变轴使能状态。"));
        case OperationKind::AutomaticTask:
            break;
        }
    }

    QString rejectionReason;
    QStringList bypassedConditions;
    switch (operation) {
    case OperationKind::ManualMotion:
        if (state.runMode != RunMode::Manual) {
            rejectionReason = QStringLiteral("当前为自动模式，请先切换到手动模式。");
            bypassedConditions.append(QStringLiteral("当前为自动模式"));
        }
        appendBypassedCondition(
            bypassedConditions,
            state.machineState == MachineState::Fault,
            QStringLiteral("机器处于故障状态"));
        appendBypassedCondition(
            bypassedConditions,
            !state.motionConnected,
            QStringLiteral("运动控制器未连接"));
        appendBypassedCondition(
            bypassedConditions,
            !state.safetyConditionsMet,
            state.safetyFailureReason);
        break;
    case OperationKind::ManualAxisEnable:
        if (state.runMode != RunMode::Manual) {
            rejectionReason = QStringLiteral("当前为自动模式，请先切换到手动模式。");
            bypassedConditions.append(QStringLiteral("当前为自动模式"));
        }
        appendBypassedCondition(
            bypassedConditions,
            state.machineState == MachineState::Fault,
            QStringLiteral("机器处于故障状态"));
        appendBypassedCondition(
            bypassedConditions,
            !state.motionConnected,
            QStringLiteral("运动控制器未连接"));
        appendBypassedCondition(
            bypassedConditions,
            !state.safetyConditionsMet,
            state.safetyFailureReason);
        break;
    case OperationKind::AutomaticTask:
        if (state.runMode != RunMode::Automatic) {
            rejectionReason = QStringLiteral("当前为手动模式，请先切换到自动模式。");
            bypassedConditions.append(QStringLiteral("当前为手动模式"));
        }
        appendBypassedCondition(
            bypassedConditions,
            state.machineState != MachineState::Ready,
            QStringLiteral("机器未就绪"));
        break;
    }

    if (!enforcesStateGuards()) {
        return OperationDecision{
            true,
            !bypassedConditions.isEmpty(),
            {},
            bypassedConditions};
    }
    if (bypassedConditions.isEmpty()) {
        return OperationDecision{true, false, {}, {}};
    }
    if (!rejectionReason.isEmpty()) {
        return rejected(rejectionReason);
    }

    switch (operation) {
    case OperationKind::ManualMotion:
        if (state.machineState == MachineState::Fault) {
            return rejected(QStringLiteral("机器处于故障状态，请先完成故障复位。"));
        }
        if (!state.motionConnected) {
            return rejected(QStringLiteral("运动控制器未连接，无法执行手动运动。"));
        }
        return rejected(state.safetyFailureReason);
    case OperationKind::ManualAxisEnable:
        if (state.machineState == MachineState::Fault) {
            return rejected(QStringLiteral("机器处于故障状态，请先完成故障复位。"));
        }
        if (!state.motionConnected) {
            return rejected(QStringLiteral("运动控制器未连接，无法改变轴使能状态。"));
        }
        return rejected(state.safetyFailureReason);
    case OperationKind::AutomaticTask:
        return rejected(QStringLiteral("机器未就绪，无法启动自动实验。"));
    }
    return rejected(QStringLiteral("当前条件不允许执行该操作。"));
}

} // namespace otms::workflow
