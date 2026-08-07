#pragma once

namespace otms::device {

class IMotionDriver
{
public:
    virtual ~IMotionDriver() = default;

    virtual int connect() = 0;
    virtual int disconnect() = 0;
    virtual int isConnected(int& connected) const = 0;

    virtual int enable(int axis) = 0;
    virtual int disable(int axis) = 0;
    virtual int isEnabled(int axis, int& enabled) const = 0;
    virtual int home(int axis, bool asynchronous = true, int timeoutSeconds = 10) = 0;

    virtual int moveAbsolute(
        int axis,
        int positionCounts,
        bool asynchronous = true,
        int timeoutSeconds = 10) = 0;
    virtual int moveRelative(
        int axis,
        int distanceCounts,
        bool asynchronous = true,
        int timeoutSeconds = 10) = 0;
    virtual int moveAbsoluteRepetitive(int axis, int positionCounts, int dwellTimeMs = 0) = 0;

    virtual int getReferencePosition(int axis, int& positionCounts) const = 0;
    virtual int getPosition(int axis, int& positionCounts) const = 0;
    virtual int getVelocity(int axis, int& velocityCountsPerSecond) const = 0;
    virtual int getMotionStatus(int axis, int& status) const = 0;

    virtual int setSpeed(int axis, int speedCountsPerSecond) = 0;
    virtual int jog(int axis, int velocityCountsPerSecond) = 0;
    virtual int stop(int axis, bool asynchronous = true, int timeoutSeconds = 10) = 0;
    virtual int abort(int axis) = 0;

    virtual bool cncBegin() = 0;
    virtual bool cncClearBuffer() = 0;
    virtual bool cncLinearAbsoluteXT(
        int xPositionCounts,
        int tPositionCounts,
        int cruiseVelocityCountsPerSecond,
        int endVelocityCountsPerSecond) = 0;
    virtual bool cncLinearAbsoluteXY(
        int xPositionCounts,
        int yPositionCounts,
        int cruiseVelocityCountsPerSecond,
        int endVelocityCountsPerSecond) = 0;
};

} // namespace otms::device
