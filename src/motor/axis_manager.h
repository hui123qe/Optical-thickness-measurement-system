#pragma once

#include "motion_driver.h"

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace otms::device {

class AxisManager
{
public:
    static AxisManager& instance();

    AxisManager(const AxisManager&) = delete;
    AxisManager& operator=(const AxisManager&) = delete;

    bool setAxisMapping(
        int logicalAxis,
        std::shared_ptr<IMotionDriver> driver,
        int controllerAxis,
        double countsPerUnit = 1.0);
    bool removeAxisMapping(int logicalAxis);
    void clearAxisMappings();
    bool hasAxisMapping(int logicalAxis) const;

    int connect(int logicalAxis);
    int disconnect(int logicalAxis);
    int isConnected(int logicalAxis, int& connected) const;
    int enable(int logicalAxis);
    int disable(int logicalAxis);
    int isEnabled(int logicalAxis, int& enabled) const;
    int home(int logicalAxis, bool asynchronous = true, int timeoutSeconds = 10);

    int setSpeed(int logicalAxis, double unitsPerSecond);
    int jog(int logicalAxis, double unitsPerSecond);
    int moveAbsolute(
        int logicalAxis,
        double position,
        bool asynchronous = true,
        int timeoutSeconds = 10);
    int moveRelative(
        int logicalAxis,
        double distance,
        bool asynchronous = true,
        int timeoutSeconds = 10);
    int moveAbsoluteRepetitive(int logicalAxis, double position, int dwellTimeMs = 0);

    int getPosition(int logicalAxis, double& position) const;
    int getReferencePosition(int logicalAxis, double& position) const;
    int getVelocity(int logicalAxis, double& unitsPerSecond) const;
    int getMotionStatus(int logicalAxis, int& status) const;

    int stop(int logicalAxis, bool asynchronous = true, int timeoutSeconds = 10);
    int abort(int logicalAxis);

    bool cncBegin(int logicalAxis);
    bool cncClearBuffer(int logicalAxis);
    bool cncLinearAbsoluteXT(
        int logicalXAxis,
        int logicalTAxis,
        double xPosition,
        double tPosition,
        double cruiseVelocity,
        double endVelocity = 0.0);
    bool cncLinearAbsoluteXY(
        int logicalXAxis,
        int logicalYAxis,
        double xPosition,
        double yPosition,
        double cruiseVelocity,
        double endVelocity = 0.0);

private:
    struct AxisMapping
    {
        std::shared_ptr<IMotionDriver> driver;
        int controllerAxis{};
        double countsPerUnit{1.0};
    };

    AxisManager() = default;

    std::optional<AxisMapping> mappingFor(int logicalAxis) const;
    static bool toCounts(double value, double countsPerUnit, int& counts);
    static double fromCounts(int counts, double countsPerUnit);
    static bool mappingsShareDriver(const AxisMapping& first, const AxisMapping& second);

    mutable std::mutex mappingsMutex_;
    std::unordered_map<int, AxisMapping> axisMappings_;
};

} // namespace otms::device
