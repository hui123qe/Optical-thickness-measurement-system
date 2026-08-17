#pragma once

#include "motion_controller.h"

#include <QByteArray>
#include <QString>

namespace otms::device {

enum class AgtioControllerType
{
    Agc301 = 0,
    Agd155 = 1,
    Agc300 = 2,
    Agm800 = 3
};

class AgtioMotionController final : public IMotionController
{
public:
    AgtioMotionController(QString ipAddress, AgtioControllerType controllerType);
    ~AgtioMotionController() override;

    AgtioMotionController(const AgtioMotionController&) = delete;
    AgtioMotionController& operator=(const AgtioMotionController&) = delete;
    AgtioMotionController(AgtioMotionController&&) = delete;
    AgtioMotionController& operator=(AgtioMotionController&&) = delete;

    int connect() override;
    int disconnect() override;
    int isConnected(int& connected) const override;

    int enableAxis(int axis) override;
    int disableAxis(int axis) override;
    int isAxisEnabled(int axis, int& enabled) const override;
    int homeAxis(int axis, bool asynchronous = true, int timeoutSeconds = 10) override;

    int moveAbsolute(
        int axis,
        int positionCounts,
        bool asynchronous = true,
        int timeoutSeconds = 10) override;
    int moveRelative(
        int axis,
        int distanceCounts,
        bool asynchronous = true,
        int timeoutSeconds = 10) override;
    int moveAbsoluteRepetitive(int axis, int positionCounts, int dwellTimeMs = 0) override;

    int getReferencePosition(int axis, int& positionCounts) const override;
    int getPosition(int axis, int& positionCounts) const override;
    int getVelocity(int axis, int& velocityCountsPerSecond) const override;
    int getMotionStatus(int axis, int& status) const override;

    int setSpeed(int axis, int speedCountsPerSecond) override;
    int jog(int axis, int velocityCountsPerSecond) override;
    int stopAxis(int axis, bool asynchronous = true, int timeoutSeconds = 10) override;
    int abortAxis(int axis) override;

    int readDigitalInput(int point, bool& value) const override;
    int readDigitalOutput(int point, bool& value) const override;
    int writeDigitalOutput(int point, bool value) override;

    bool cncBegin() override;
    bool cncClearBuffer() override;
    bool cncLinearAbsoluteXT(
        int xPositionCounts,
        int tPositionCounts,
        int cruiseVelocityCountsPerSecond,
        int endVelocityCountsPerSecond) override;
    bool cncLinearAbsoluteXY(
        int xPositionCounts,
        int yPositionCounts,
        int cruiseVelocityCountsPerSecond,
        int endVelocityCountsPerSecond) override;

    QString ipAddress() const;
    AgtioControllerType controllerType() const;

private:
    bool hasController() const;

    QString ipAddress_;
    QByteArray ipAddressBytes_;
    AgtioControllerType controllerType_;
    char* controllerId_{}; // The SDK exposes no controller-destruction function.
};

} // namespace otms::device
