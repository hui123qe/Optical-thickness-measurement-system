#pragma once

namespace otms::device {

class IMotionController
{
public:
    virtual ~IMotionController() = default;

    virtual int connect() = 0;
    virtual int disconnect() = 0;
    virtual int isConnected(int& connected) const = 0;

    virtual int enableAxis(int axis) = 0;
    virtual int disableAxis(int axis) = 0;
    virtual int isAxisEnabled(int axis, int& enabled) const = 0;
    virtual int homeAxis(int axis, bool asynchronous = true, int timeoutSeconds = 10) = 0;

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
    virtual int stopAxis(int axis, bool asynchronous = true, int timeoutSeconds = 10) = 0;
    virtual int abortAxis(int axis) = 0;

    virtual int readDigitalInput(int point, bool& value) const = 0;
    virtual int readDigitalOutput(int point, bool& value) const = 0;
    virtual int writeDigitalOutput(int point, bool value) = 0;

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
