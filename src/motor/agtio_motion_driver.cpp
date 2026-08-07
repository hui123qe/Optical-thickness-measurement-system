#include "agtio_motion_driver.h"

#include <ExAKBMotion.h>

#include <QDebug>
#include <QLoggingCategory>

#include <utility>

Q_LOGGING_CATEGORY(agtioMotionLog, "otms.device.agtio")

namespace otms::device {

AgtioMotionDriver::AgtioMotionDriver(QString ipAddress, AgtioControllerType controllerType)
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

AgtioMotionDriver::~AgtioMotionDriver()
{
    if (hasController() && GetIsConnected(controllerId_) == 1) {
        qCInfo(agtioMotionLog) << "Disconnecting AGTIO controller during driver shutdown";
        Disconnect(controllerId_);
    }
}

int AgtioMotionDriver::connect()
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

int AgtioMotionDriver::disconnect()
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

int AgtioMotionDriver::isConnected(int& connected) const
{
    if (!hasController()) {
        connected = 0;
        return 0;
    }

    connected = GetIsConnected(controllerId_);
    return 1;
}

int AgtioMotionDriver::enable(int axis)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Enabling AGTIO axis" << axis;
    return MotorOn(controllerId_, axis);
}

int AgtioMotionDriver::disable(int axis)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Disabling AGTIO axis" << axis;
    return MotorOff(controllerId_, axis);
}

int AgtioMotionDriver::isEnabled(int axis, int& enabled) const
{
    return hasController() ? GetMotorOnStatus(controllerId_, axis, enabled) : 0;
}

int AgtioMotionDriver::home(int axis, bool asynchronous, int timeoutSeconds)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Homing AGTIO axis" << axis << "asynchronous" << asynchronous
                           << "timeoutSeconds" << timeoutSeconds;
    return Homingon(controllerId_, axis, asynchronous, timeoutSeconds);
}

int AgtioMotionDriver::moveAbsolute(
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

int AgtioMotionDriver::moveRelative(
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

int AgtioMotionDriver::moveAbsoluteRepetitive(int axis, int positionCounts, int dwellTimeMs)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Starting repetitive AGTIO absolute motion" << axis
                           << positionCounts << "dwellTimeMs" << dwellTimeMs;
    return MoveAbsRepetitive(controllerId_, axis, positionCounts, dwellTimeMs);
}

int AgtioMotionDriver::getReferencePosition(int axis, int& positionCounts) const
{
    return hasController() ? GetPosRef(controllerId_, axis, positionCounts) : 0;
}

int AgtioMotionDriver::getPosition(int axis, int& positionCounts) const
{
    return hasController() ? GetPos(controllerId_, axis, positionCounts) : 0;
}

int AgtioMotionDriver::getVelocity(int axis, int& velocityCountsPerSecond) const
{
    return hasController() ? ReadVel(controllerId_, axis, velocityCountsPerSecond) : 0;
}

int AgtioMotionDriver::getMotionStatus(int axis, int& status) const
{
    return hasController() ? GetMotionStat(controllerId_, axis, status) : 0;
}

int AgtioMotionDriver::setSpeed(int axis, int speedCountsPerSecond)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Setting AGTIO axis speed" << axis << speedCountsPerSecond;
    SetSpeed(controllerId_, axis, speedCountsPerSecond);
    return 1;
}

int AgtioMotionDriver::jog(int axis, int velocityCountsPerSecond)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Jogging AGTIO axis" << axis << velocityCountsPerSecond;
    return Jog(controllerId_, axis, velocityCountsPerSecond);
}

int AgtioMotionDriver::stop(int axis, bool asynchronous, int timeoutSeconds)
{
    if (!hasController()) {
        return 0;
    }
    qCInfo(agtioMotionLog) << "Stopping AGTIO axis" << axis << "asynchronous" << asynchronous
                           << "timeoutSeconds" << timeoutSeconds;
    return Stop(controllerId_, axis, asynchronous, timeoutSeconds);
}

int AgtioMotionDriver::abort(int axis)
{
    if (!hasController()) {
        return 0;
    }
    qCWarning(agtioMotionLog) << "Aborting AGTIO axis" << axis;
    return Abort(controllerId_, axis);
}

bool AgtioMotionDriver::cncBegin()
{
    if (!hasController()) {
        return false;
    }
    qCInfo(agtioMotionLog) << "Starting AGTIO CNC buffer";
    return CNCBegin(controllerId_);
}

bool AgtioMotionDriver::cncClearBuffer()
{
    if (!hasController()) {
        return false;
    }
    qCInfo(agtioMotionLog) << "Clearing AGTIO CNC buffer";
    return CNCClearBuffer(controllerId_);
}

bool AgtioMotionDriver::cncLinearAbsoluteXT(
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

bool AgtioMotionDriver::cncLinearAbsoluteXY(
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

QString AgtioMotionDriver::ipAddress() const
{
    return ipAddress_;
}

AgtioControllerType AgtioMotionDriver::controllerType() const
{
    return controllerType_;
}

bool AgtioMotionDriver::hasController() const
{
    return controllerId_ != nullptr;
}

} // namespace otms::device
