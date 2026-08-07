#pragma once

#include "motion_driver.h"

#include <chrono>
#include <mutex>
#include <unordered_map>

namespace otms::device {

class VirtualMotionDriver final : public IMotionDriver
{
public:
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

private:
    struct AxisState
    {
        int positionCounts{};
        int referencePositionCounts{};
        int velocityCountsPerSecond{};
        int configuredSpeedCountsPerSecond{};
        int targetPositionCounts{};
        bool enabled{};
        bool inPosition{true};
        bool movePending{};
        std::chrono::steady_clock::time_point arrivalDeadline{};
    };

    [[nodiscard]] static bool validAxis(int axis);
    [[nodiscard]] bool axisReady(int axis) const;
    AxisState& stateFor(int axis);
    AxisState* findMutableState(int axis) const;
    [[nodiscard]] const AxisState* findState(int axis) const;
    static void updateArrival(AxisState& state);
    bool moveImmediate(int axis, int positionCounts);

    mutable std::mutex stateMutex_;
    mutable std::unordered_map<int, AxisState> axisStates_;
    bool connected_{};
    bool cncActive_{};
};

} // namespace otms::device
