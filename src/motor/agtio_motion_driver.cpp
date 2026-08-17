#include "agtio_motion_driver.h"

#include <ExAKBMotion.h>

#include <QDebug>
#include <QLoggingCategory>

#include <utility>

Q_LOGGING_CATEGORY(agtioMotionLog, "otms.device.agtio")

namespace otms::device {

AgtioMotionController::AgtioMotionController(QString ipAddress, AgtioControllerType controllerType)
    : ipAddress_(std::move(ipAddress))
    , ipAddressBytes_(ipAddress_.toLatin1())
    , controllerType_(controllerType)
    , controllerId_(CreateController(static_cast<int>(controllerType_)))
{
    if (!hasController()) {
        qCWarning(agtioMotionLog) << "AGTIO controller creation failed for type"
                                 << static_cast<int>(controllerType_);
    }
}

AgtioMotionController::~AgtioMotionController()
{
    if (hasController() && GetIsConnected(controllerId_) == 1) {
        qCInfo(agtioMotionLog) << "Disconnecting AGTIO controller during driver shutdown";
        Disconnect(controllerId_);
    }
}

int AgtioMotionController::connect()
{
    if (!hasController()) {
        return 0;
    }
    if (GetIsConnected(controllerId_) == 1) {
        return 1;
    }

    qCInfo(agtioMotionLog) << "Connecting AGTIO controller at" << ipAddress_;
    return Connect(controllerId_, ipAddressBytes_.data());
}

int AgtioMotionController::disconnect()
{
    if (!hasController()) {
        return 0;
    }
    if (GetIsConnected(controllerId_) == 0) {
        return 1;
    }

    qCInfo(agtioMotionLog) << "Disconnecting AGTIO controller at" << ipAddress_;
    return Disconnect(controllerId_);
}

int AgtioMotionController::isConnected(int& connected) const
{
    if (!hasController()) {
        connected = 0;
        return 0;
    }

    connected = GetIsConnected(controllerId_);
    return 1;
}

int AgtioMotionController::enableAxis(int axis)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Enabling AGTIO axis" << axis;
    return MotorOn(controllerId_, axis);
}

int AgtioMotionController::disableAxis(int axis)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Disabling AGTIO axis" << axis;
    return MotorOff(controllerId_, axis);
}

int AgtioMotionController::isAxisEnabled(int axis, int& enabled) const
{
    return hasController() ? GetMotorOnStatus(controllerId_, axis, enabled) : 0;
}

int AgtioMotionController::homeAxis(int axis, bool asynchronous, int timeoutSeconds)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Homing AGTIO axis" << axis << "asynchronous" << asynchronous
                           << "timeoutSeconds" << timeoutSeconds;
    return Homingon(controllerId_, axis, asynchronous, timeoutSeconds);
}

int AgtioMotionController::moveAbsolute(
    int axis,
    int positionCounts,
    bool asynchronous,
    int timeoutSeconds)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Moving AGTIO axis absolute" << axis << positionCounts
                           << "asynchronous" << asynchronous << "timeoutSeconds" << timeoutSeconds;
    return MoveAbs(controllerId_, axis, positionCounts, asynchronous, timeoutSeconds);
}

int AgtioMotionController::moveRelative(
    int axis,
    int distanceCounts,
    bool asynchronous,
    int timeoutSeconds)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Moving AGTIO axis relative" << axis << distanceCounts
                           << "asynchronous" << asynchronous << "timeoutSeconds" << timeoutSeconds;
    return MoveRel(controllerId_, axis, distanceCounts, asynchronous, timeoutSeconds);
}

int AgtioMotionController::moveAbsoluteRepetitive(int axis, int positionCounts, int dwellTimeMs)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Starting repetitive AGTIO absolute motion" << axis
                           << positionCounts << "dwellTimeMs" << dwellTimeMs;
    return MoveAbsRepetitive(controllerId_, axis, positionCounts, dwellTimeMs);
}

int AgtioMotionController::getReferencePosition(int axis, int& positionCounts) const
{
    return hasController() ? GetPosRef(controllerId_, axis, positionCounts) : 0;
}

int AgtioMotionController::getPosition(int axis, int& positionCounts) const
{
    return hasController() ? GetPos(controllerId_, axis, positionCounts) : 0;
}

int AgtioMotionController::getVelocity(int axis, int& velocityCountsPerSecond) const
{
    return hasController() ? ReadVel(controllerId_, axis, velocityCountsPerSecond) : 0;
}

int AgtioMotionController::getMotionStatus(int axis, int& status) const
{
    return hasController() ? GetMotionStat(controllerId_, axis, status) : 0;
}

int AgtioMotionController::setSpeed(int axis, int speedCountsPerSecond)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Setting AGTIO axis speed" << axis << speedCountsPerSecond;
    SetSpeed(controllerId_, axis, speedCountsPerSecond);
    return 1;
}

int AgtioMotionController::jog(int axis, int velocityCountsPerSecond)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Jogging AGTIO axis" << axis << velocityCountsPerSecond;
    return Jog(controllerId_, axis, velocityCountsPerSecond);
}

int AgtioMotionController::stopAxis(int axis, bool asynchronous, int timeoutSeconds)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Stopping AGTIO axis" << axis << "asynchronous" << asynchronous
                           << "timeoutSeconds" << timeoutSeconds;
    return Stop(controllerId_, axis, asynchronous, timeoutSeconds);
}

int AgtioMotionController::abortAxis(int axis)
{
    if (!hasController()) {
        return 0;
    }
    qCWarning(agtioMotionLog) << "Aborting AGTIO axis" << axis;
    return Abort(controllerId_, axis);
}

int AgtioMotionController::readDigitalInput(int point, bool& value) const
{
    int rawValue = 0;
    const int result = hasController() ? GetDInPort(controllerId_, point, rawValue) : 0;
    if (result == 1) {
        value = rawValue != 0;
    }
    return result;
}

int AgtioMotionController::readDigitalOutput(int point, bool& value) const
{
    int rawValue = 0;
    const int result = hasController() ? GetDOutPort(controllerId_, point, rawValue) : 0;
    if (result == 1) {
        value = rawValue != 0;
    }
    return result;
}

int AgtioMotionController::writeDigitalOutput(int point, bool value)
{
    if (!hasController() || GetIsConnected(controllerId_) != 1) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Writing AGTIO digital output" << point << value;
    SetDOutPort(controllerId_, point, value);
    return 1;
}

bool AgtioMotionController::cncBegin()
{
    if (!hasController()) {
        return false;
    }
    qCInfo(agtioMotionLog) << "Starting AGTIO CNC buffer";
    return CNCBegin(controllerId_);
}

bool AgtioMotionController::cncClearBuffer()
{
    if (!hasController()) {
        return false;
    }
    qCInfo(agtioMotionLog) << "Clearing AGTIO CNC buffer";
    return CNCClearBuffer(controllerId_);
}

bool AgtioMotionController::cncLinearAbsoluteXT(
    int xPositionCounts,
    int tPositionCounts,
    int cruiseVelocityCountsPerSecond,
    int endVelocityCountsPerSecond)
{
    if (!hasController()) {
        return false;
    }
    qCInfo(agtioMotionLog) << "Queueing AGTIO CNC XT motion" << xPositionCounts << tPositionCounts
                           << cruiseVelocityCountsPerSecond << endVelocityCountsPerSecond;
    return CNCLinearAbsoluteXT(
        controllerId_,
        xPositionCounts,
        tPositionCounts,
        cruiseVelocityCountsPerSecond,
        endVelocityCountsPerSecond);
}

bool AgtioMotionController::cncLinearAbsoluteXY(
    int xPositionCounts,
    int yPositionCounts,
    int cruiseVelocityCountsPerSecond,
    int endVelocityCountsPerSecond)
{
    if (!hasController()) {
        return false;
    }
    qCInfo(agtioMotionLog) << "Queueing AGTIO CNC XY motion" << xPositionCounts << yPositionCounts
                           << cruiseVelocityCountsPerSecond << endVelocityCountsPerSecond;
    return CNCLinearAbsoluteXY(
        controllerId_,
        xPositionCounts,
        yPositionCounts,
        cruiseVelocityCountsPerSecond,
        endVelocityCountsPerSecond);
}

QString AgtioMotionController::ipAddress() const
{
    return ipAddress_;
}

AgtioControllerType AgtioMotionController::controllerType() const
{
    return controllerType_;
}

bool AgtioMotionController::hasController() const
{
    return controllerId_ != nullptr;
}

} // namespace otms::device
