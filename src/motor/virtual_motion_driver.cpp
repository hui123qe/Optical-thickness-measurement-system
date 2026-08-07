#include "virtual_motion_driver.h"

#include <limits>

namespace otms::device {
namespace {

constexpr int VirtualXAxis = 0;
constexpr int VirtualSecondAxis = 1;
constexpr auto VirtualMoveDuration = std::chrono::milliseconds(300);

bool additionFitsInt(int first, int second)
{
    return (second <= 0 || first <= std::numeric_limits<int>::max() - second)
        && (second >= 0 || first >= std::numeric_limits<int>::min() - second);
}

} // namespace

int VirtualMotionDriver::connect()
{
    std::scoped_lock lock(stateMutex_);
    connected_ = true;
    return 1;
}

int VirtualMotionDriver::disconnect()
{
    std::scoped_lock lock(stateMutex_);
    for (auto& [axis, state] : axisStates_) {
        static_cast<void>(axis);
        state.enabled = false;
        state.velocityCountsPerSecond = 0;
        state.inPosition = true;
        state.movePending = false;
    }
    cncActive_ = false;
    connected_ = false;
    return 1;
}

int VirtualMotionDriver::isConnected(int& connected) const
{
    std::scoped_lock lock(stateMutex_);
    connected = connected_ ? 1 : 0;
    return 1;
}

int VirtualMotionDriver::enable(int axis)
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        return 0;
    }
    stateFor(axis).enabled = true;
    return 1;
}

int VirtualMotionDriver::disable(int axis)
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        return 0;
    }
    AxisState& state = stateFor(axis);
    state.enabled = false;
    state.velocityCountsPerSecond = 0;
    state.inPosition = true;
    state.movePending = false;
    return 1;
}

int VirtualMotionDriver::isEnabled(int axis, int& enabled) const
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        enabled = 0;
        return 0;
    }
    const AxisState* state = findState(axis);
    enabled = state != nullptr && state->enabled ? 1 : 0;
    return 1;
}

int VirtualMotionDriver::home(int axis, bool asynchronous, int timeoutSeconds)
{
    static_cast<void>(asynchronous);
    if (timeoutSeconds <= 0) {
        return 0;
    }
    std::scoped_lock lock(stateMutex_);
    return moveImmediate(axis, 0) ? 1 : 0;
}

int VirtualMotionDriver::moveAbsolute(
    int axis,
    int positionCounts,
    bool asynchronous,
    int timeoutSeconds)
{
    static_cast<void>(asynchronous);
    if (timeoutSeconds <= 0) {
        return 0;
    }
    std::scoped_lock lock(stateMutex_);
    return moveImmediate(axis, positionCounts) ? 1 : 0;
}

int VirtualMotionDriver::moveRelative(
    int axis,
    int distanceCounts,
    bool asynchronous,
    int timeoutSeconds)
{
    static_cast<void>(asynchronous);
    if (timeoutSeconds <= 0) {
        return 0;
    }

    std::scoped_lock lock(stateMutex_);
    if (!axisReady(axis)) {
        return 0;
    }
    AxisState& state = stateFor(axis);
    updateArrival(state);
    if (!additionFitsInt(state.positionCounts, distanceCounts)) {
        return 0;
    }
    return moveImmediate(axis, state.positionCounts + distanceCounts) ? 1 : 0;
}

int VirtualMotionDriver::moveAbsoluteRepetitive(
    int axis,
    int positionCounts,
    int dwellTimeMs)
{
    if (dwellTimeMs < 0) {
        return 0;
    }
    std::scoped_lock lock(stateMutex_);
    return moveImmediate(axis, positionCounts) ? 1 : 0;
}

int VirtualMotionDriver::getReferencePosition(int axis, int& positionCounts) const
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        return 0;
    }
    AxisState* state = findMutableState(axis);
    if (state != nullptr) {
        updateArrival(*state);
    }
    positionCounts = state != nullptr ? state->referencePositionCounts : 0;
    return 1;
}

int VirtualMotionDriver::getPosition(int axis, int& positionCounts) const
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        return 0;
    }
    AxisState* state = findMutableState(axis);
    if (state != nullptr) {
        updateArrival(*state);
    }
    positionCounts = state != nullptr ? state->positionCounts : 0;
    return 1;
}

int VirtualMotionDriver::getVelocity(int axis, int& velocityCountsPerSecond) const
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        return 0;
    }
    AxisState* state = findMutableState(axis);
    if (state != nullptr) {
        updateArrival(*state);
    }
    velocityCountsPerSecond = state != nullptr ? state->velocityCountsPerSecond : 0;
    return 1;
}

int VirtualMotionDriver::getMotionStatus(int axis, int& status) const
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        status = 0;
        return 0;
    }
    AxisState* state = findMutableState(axis);
    if (state != nullptr) {
        updateArrival(*state);
    }
    status = state == nullptr || state->inPosition ? 1 : 0;
    return 1;
}

int VirtualMotionDriver::setSpeed(int axis, int speedCountsPerSecond)
{
    std::scoped_lock lock(stateMutex_);
    if (!axisReady(axis) || speedCountsPerSecond < 0) {
        return 0;
    }
    stateFor(axis).configuredSpeedCountsPerSecond = speedCountsPerSecond;
    return 1;
}

int VirtualMotionDriver::jog(int axis, int velocityCountsPerSecond)
{
    std::scoped_lock lock(stateMutex_);
    if (!axisReady(axis) || velocityCountsPerSecond == 0) {
        return 0;
    }
    AxisState& state = stateFor(axis);
    state.movePending = false;
    state.velocityCountsPerSecond = velocityCountsPerSecond;
    state.inPosition = false;
    return 1;
}

int VirtualMotionDriver::stop(int axis, bool asynchronous, int timeoutSeconds)
{
    static_cast<void>(asynchronous);
    if (timeoutSeconds <= 0) {
        return 0;
    }
    return abort(axis);
}

int VirtualMotionDriver::abort(int axis)
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_ || !validAxis(axis)) {
        return 0;
    }
    AxisState& state = stateFor(axis);
    state.movePending = false;
    state.velocityCountsPerSecond = 0;
    state.inPosition = true;
    return 1;
}

bool VirtualMotionDriver::cncBegin()
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_) {
        return false;
    }
    cncActive_ = true;
    return true;
}

bool VirtualMotionDriver::cncClearBuffer()
{
    std::scoped_lock lock(stateMutex_);
    if (!connected_) {
        return false;
    }
    cncActive_ = false;
    return true;
}

bool VirtualMotionDriver::cncLinearAbsoluteXT(
    int xPositionCounts,
    int tPositionCounts,
    int cruiseVelocityCountsPerSecond,
    int endVelocityCountsPerSecond)
{
    static_cast<void>(cruiseVelocityCountsPerSecond);
    static_cast<void>(endVelocityCountsPerSecond);
    std::scoped_lock lock(stateMutex_);
    return cncActive_
        && moveImmediate(VirtualXAxis, xPositionCounts)
        && moveImmediate(VirtualSecondAxis, tPositionCounts);
}

bool VirtualMotionDriver::cncLinearAbsoluteXY(
    int xPositionCounts,
    int yPositionCounts,
    int cruiseVelocityCountsPerSecond,
    int endVelocityCountsPerSecond)
{
    static_cast<void>(cruiseVelocityCountsPerSecond);
    static_cast<void>(endVelocityCountsPerSecond);
    std::scoped_lock lock(stateMutex_);
    return cncActive_
        && moveImmediate(VirtualXAxis, xPositionCounts)
        && moveImmediate(VirtualSecondAxis, yPositionCounts);
}

bool VirtualMotionDriver::validAxis(int axis)
{
    return axis >= 0;
}

bool VirtualMotionDriver::axisReady(int axis) const
{
    if (!connected_ || !validAxis(axis)) {
        return false;
    }
    const AxisState* state = findState(axis);
    return state != nullptr && state->enabled;
}

VirtualMotionDriver::AxisState& VirtualMotionDriver::stateFor(int axis)
{
    return axisStates_[axis];
}

const VirtualMotionDriver::AxisState* VirtualMotionDriver::findState(int axis) const
{
    const auto state = axisStates_.find(axis);
    return state != axisStates_.end() ? &state->second : nullptr;
}

VirtualMotionDriver::AxisState* VirtualMotionDriver::findMutableState(int axis) const
{
    const auto state = axisStates_.find(axis);
    return state != axisStates_.end() ? &state->second : nullptr;
}

void VirtualMotionDriver::updateArrival(AxisState& state)
{
    if (!state.movePending || std::chrono::steady_clock::now() < state.arrivalDeadline) {
        return;
    }

    state.positionCounts = state.targetPositionCounts;
    state.referencePositionCounts = state.targetPositionCounts;
    state.velocityCountsPerSecond = 0;
    state.movePending = false;
    state.inPosition = true;
}

bool VirtualMotionDriver::moveImmediate(int axis, int positionCounts)
{
    if (!axisReady(axis)) {
        return false;
    }

    AxisState& state = stateFor(axis);
    state.targetPositionCounts = positionCounts;
    state.arrivalDeadline = std::chrono::steady_clock::now() + VirtualMoveDuration;
    state.movePending = true;
    state.inPosition = false;
    state.velocityCountsPerSecond = state.configuredSpeedCountsPerSecond;
    return true;
}

} // namespace otms::device
