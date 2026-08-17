#pragma once

#include "agtio_motion_driver.h"
#include "motion_controller.h"

#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include <QString>
#include <QStringList>

namespace otms::device {

enum class LogicalAxis
{
    X,
    Y,
    Z
};

enum class StackLampColor
{
    Red,
    Yellow,
    Green
};

class MotionControllerManager
{
public:
    static MotionControllerManager& instance();

    MotionControllerManager(const MotionControllerManager&) = delete;
    MotionControllerManager& operator=(const MotionControllerManager&) = delete;

    bool initialize(QString* errorMessage = nullptr);
    void shutdown();
    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] QStringList controllerIds() const;

    int connectController(const QString& controllerId);
    int disconnectController(const QString& controllerId);
    int isControllerConnected(const QString& controllerId, int& connected) const;
    int isAxisControllerConnected(LogicalAxis logicalAxis, int& connected) const;

    int enableAxis(LogicalAxis logicalAxis);
    int disableAxis(LogicalAxis logicalAxis);
    int isAxisEnabled(LogicalAxis logicalAxis, int& enabled) const;
    int homeAxis(
        LogicalAxis logicalAxis,
        bool asynchronous = true,
        int timeoutSeconds = 10);

    int setSpeed(LogicalAxis logicalAxis, double unitsPerSecond);
    int jog(LogicalAxis logicalAxis, double unitsPerSecond);
    int moveAbsolute(
        LogicalAxis logicalAxis,
        double position,
        bool asynchronous = true,
        int timeoutSeconds = 10);
    int moveRelative(
        LogicalAxis logicalAxis,
        double distance,
        bool asynchronous = true,
        int timeoutSeconds = 10);
    int moveAbsoluteRepetitive(
        LogicalAxis logicalAxis,
        double position,
        int dwellTimeMs = 0);

    int getPosition(LogicalAxis logicalAxis, double& position) const;
    int getReferencePosition(LogicalAxis logicalAxis, double& position) const;
    int getVelocity(LogicalAxis logicalAxis, double& unitsPerSecond) const;
    int getMotionStatus(LogicalAxis logicalAxis, int& status) const;

    int stopAxis(
        LogicalAxis logicalAxis,
        bool asynchronous = true,
        int timeoutSeconds = 10);
    int abortAxis(LogicalAxis logicalAxis);

    int readDigitalInput(int logicalInput, bool& value) const;
    int readDigitalOutput(int logicalOutput, bool& value) const;
    int writeDigitalOutput(int logicalOutput, bool value);

    int readDoorLocked(bool& locked) const;
    int readLightCurtainClear(bool& clear) const;
    int setDoorLocked(bool locked);
    int setStackLamp(StackLampColor color, bool enabled);

    bool cncBegin(LogicalAxis logicalAxis);
    bool cncClearBuffer(LogicalAxis logicalAxis);
    bool cncLinearAbsoluteXT(
        LogicalAxis logicalXAxis,
        LogicalAxis logicalTAxis,
        double xPosition,
        double tPosition,
        double cruiseVelocity,
        double endVelocity = 0.0);
    bool cncLinearAbsoluteXY(
        LogicalAxis logicalXAxis,
        LogicalAxis logicalYAxis,
        double xPosition,
        double yPosition,
        double cruiseVelocity,
        double endVelocity = 0.0);

private:
    struct ControllerEntry
    {
        std::unique_ptr<IMotionController> controller;
        mutable std::mutex commandMutex;
    };

    struct AxisMapping
    {
        QString controllerId;
        int controllerAxis{};
        double countsPerUnit{1.0};
    };

    struct IoMapping
    {
        QString controllerId;
        int controllerPoint{};
        bool activeHigh{true};
    };

    MotionControllerManager() = default;
    ~MotionControllerManager();

    [[nodiscard]] ControllerEntry* controllerFor(const QString& controllerId) const;
    [[nodiscard]] const AxisMapping* axisMappingFor(LogicalAxis logicalAxis) const;
    int readDigitalInput(const QString& logicalInput, bool& value) const;
    int readDigitalOutput(const QString& logicalOutput, bool& value) const;
    int writeDigitalOutput(const QString& logicalOutput, bool value);
    static bool toCounts(double value, double countsPerUnit, int& counts);
    static double fromCounts(int counts, double countsPerUnit);

    mutable std::shared_mutex registryMutex_;
    std::map<QString, std::unique_ptr<ControllerEntry>> controllers_;
    std::unordered_map<LogicalAxis, AxisMapping> axisMappings_;
    std::map<QString, IoMapping> digitalInputMappings_;
    std::map<QString, IoMapping> digitalOutputMappings_;
    bool initialized_{};
};

} // namespace otms::device
