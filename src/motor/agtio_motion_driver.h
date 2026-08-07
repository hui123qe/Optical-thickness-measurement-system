#pragma once

#include "motion_driver.h"

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

class AgtioMotionDriver final : public IMotionDriver
{
public:
    AgtioMotionDriver(QString ipAddress, AgtioControllerType controllerType);
    ~AgtioMotionDriver() override;

    AgtioMotionDriver(const AgtioMotionDriver&) = delete;
    AgtioMotionDriver& operator=(const AgtioMotionDriver&) = delete;
    AgtioMotionDriver(AgtioMotionDriver&&) = delete;
    AgtioMotionDriver& operator=(AgtioMotionDriver&&) = delete;

    int connect() override;
    int disconnect() override;
    int isConnected(int& connected) const override;

    int enable(int axis) override;
    int disable(int axis) override;
    int isEnabled(int axis, int& enabled) const override;
    int home(int axis, bool asynchronous = true, int timeoutSeconds = 10) override;

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
    int stop(int axis, bool asynchronous = true, int timeoutSeconds = 10) override;
    int abort(int axis) override;

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
