#include "axis_manager.h"

#include <cmath>
#include <limits>
#include <utility>

namespace otms::device {

AxisManager& AxisManager::instance()
{
    static AxisManager manager;
    return manager;
}

bool AxisManager::setAxisMapping(
    int logicalAxis,
    std::shared_ptr<IMotionDriver> driver,
    int controllerAxis,
    double countsPerUnit)
{
    if (!driver || controllerAxis < 0 || !std::isfinite(countsPerUnit) || countsPerUnit <= 0.0) {
        return false;
    }

    std::scoped_lock lock(mappingsMutex_);
    axisMappings_.insert_or_assign(
        logicalAxis,
        AxisMapping{std::move(driver), controllerAxis, countsPerUnit});
    return true;
}

bool AxisManager::removeAxisMapping(int logicalAxis)
{
    std::scoped_lock lock(mappingsMutex_);
    return axisMappings_.erase(logicalAxis) == 1;
}

void AxisManager::clearAxisMappings()
{
    std::scoped_lock lock(mappingsMutex_);
    axisMappings_.clear();
}

bool AxisManager::hasAxisMapping(int logicalAxis) const
{
    std::scoped_lock lock(mappingsMutex_);
    return axisMappings_.contains(logicalAxis);
}

int AxisManager::connect(int logicalAxis)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->connect() : 0;
}

int AxisManager::disconnect(int logicalAxis)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->disconnect() : 0;
}

int AxisManager::isConnected(int logicalAxis, int& connected) const
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->isConnected(connected) : 0;
}

int AxisManager::enable(int logicalAxis)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->enable(mapping->controllerAxis) : 0;
}

int AxisManager::disable(int logicalAxis)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->disable(mapping->controllerAxis) : 0;
}

int AxisManager::isEnabled(int logicalAxis, int& enabled) const
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->isEnabled(mapping->controllerAxis, enabled) : 0;
}

int AxisManager::home(int logicalAxis, bool asynchronous, int timeoutSeconds)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping
        ? mapping->driver->home(mapping->controllerAxis, asynchronous, timeoutSeconds)
        : 0;
}

int AxisManager::setSpeed(int logicalAxis, double unitsPerSecond)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int speedCountsPerSecond = 0;
    if (!mapping || !toCounts(unitsPerSecond, mapping->countsPerUnit, speedCountsPerSecond)) {
        return 0;
    }
    return mapping->driver->setSpeed(mapping->controllerAxis, speedCountsPerSecond);
}

int AxisManager::jog(int logicalAxis, double unitsPerSecond)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int velocityCountsPerSecond = 0;
    if (!mapping || !toCounts(unitsPerSecond, mapping->countsPerUnit, velocityCountsPerSecond)) {
        return 0;
    }
    return mapping->driver->jog(mapping->controllerAxis, velocityCountsPerSecond);
}

int AxisManager::moveAbsolute(
    int logicalAxis,
    double position,
    bool asynchronous,
    int timeoutSeconds)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int positionCounts = 0;
    if (!mapping || !toCounts(position, mapping->countsPerUnit, positionCounts)) {
        return 0;
    }
    return mapping->driver->moveAbsolute(
        mapping->controllerAxis,
        positionCounts,
        asynchronous,
        timeoutSeconds);
}

int AxisManager::moveRelative(
    int logicalAxis,
    double distance,
    bool asynchronous,
    int timeoutSeconds)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int distanceCounts = 0;
    if (!mapping || !toCounts(distance, mapping->countsPerUnit, distanceCounts)) {
        return 0;
    }
    return mapping->driver->moveRelative(
        mapping->controllerAxis,
        distanceCounts,
        asynchronous,
        timeoutSeconds);
}

int AxisManager::moveAbsoluteRepetitive(int logicalAxis, double position, int dwellTimeMs)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int positionCounts = 0;
    if (!mapping || !toCounts(position, mapping->countsPerUnit, positionCounts)) {
        return 0;
    }
    return mapping->driver->moveAbsoluteRepetitive(
        mapping->controllerAxis,
        positionCounts,
        dwellTimeMs);
}

int AxisManager::getPosition(int logicalAxis, double& position) const
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int positionCounts = 0;
    if (!mapping) {
        return 0;
    }
    const int result = mapping->driver->getPosition(mapping->controllerAxis, positionCounts);
    if (result == 1) {
        position = fromCounts(positionCounts, mapping->countsPerUnit);
    }
    return result;
}

int AxisManager::getReferencePosition(int logicalAxis, double& position) const
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int positionCounts = 0;
    if (!mapping) {
        return 0;
    }
    const int result = mapping->driver->getReferencePosition(mapping->controllerAxis, positionCounts);
    if (result == 1) {
        position = fromCounts(positionCounts, mapping->countsPerUnit);
    }
    return result;
}

int AxisManager::getVelocity(int logicalAxis, double& unitsPerSecond) const
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    int velocityCountsPerSecond = 0;
    if (!mapping) {
        return 0;
    }
    const int result = mapping->driver->getVelocity(
        mapping->controllerAxis,
        velocityCountsPerSecond);
    if (result == 1) {
        unitsPerSecond = fromCounts(velocityCountsPerSecond, mapping->countsPerUnit);
    }
    return result;
}

int AxisManager::getMotionStatus(int logicalAxis, int& status) const
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->getMotionStatus(mapping->controllerAxis, status) : 0;
}

int AxisManager::stop(int logicalAxis, bool asynchronous, int timeoutSeconds)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping
        ? mapping->driver->stop(mapping->controllerAxis, asynchronous, timeoutSeconds)
        : 0;
}

int AxisManager::abort(int logicalAxis)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping ? mapping->driver->abort(mapping->controllerAxis) : 0;
}

bool AxisManager::cncBegin(int logicalAxis)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping && mapping->driver->cncBegin();
}

bool AxisManager::cncClearBuffer(int logicalAxis)
{
    const std::optional<AxisMapping> mapping = mappingFor(logicalAxis);
    return mapping && mapping->driver->cncClearBuffer();
}

bool AxisManager::cncLinearAbsoluteXT(
    int logicalXAxis,
    int logicalTAxis,
    double xPosition,
    double tPosition,
    double cruiseVelocity,
    double endVelocity)
{
    const std::optional<AxisMapping> xMapping = mappingFor(logicalXAxis);
    const std::optional<AxisMapping> tMapping = mappingFor(logicalTAxis);
    int xPositionCounts = 0;
    int tPositionCounts = 0;
    int cruiseVelocityCountsPerSecond = 0;
    int endVelocityCountsPerSecond = 0;
    if (!xMapping || !tMapping || !mappingsShareDriver(*xMapping, *tMapping)
        || !toCounts(xPosition, xMapping->countsPerUnit, xPositionCounts)
        || !toCounts(tPosition, tMapping->countsPerUnit, tPositionCounts)
        || !toCounts(cruiseVelocity, xMapping->countsPerUnit, cruiseVelocityCountsPerSecond)
        || !toCounts(endVelocity, xMapping->countsPerUnit, endVelocityCountsPerSecond)) {
        return false;
    }

    return xMapping->driver->cncLinearAbsoluteXT(
        xPositionCounts,
        tPositionCounts,
        cruiseVelocityCountsPerSecond,
        endVelocityCountsPerSecond);
}

bool AxisManager::cncLinearAbsoluteXY(
    int logicalXAxis,
    int logicalYAxis,
    double xPosition,
    double yPosition,
    double cruiseVelocity,
    double endVelocity)
{
    const std::optional<AxisMapping> xMapping = mappingFor(logicalXAxis);
    const std::optional<AxisMapping> yMapping = mappingFor(logicalYAxis);
    int xPositionCounts = 0;
    int yPositionCounts = 0;
    int cruiseVelocityCountsPerSecond = 0;
    int endVelocityCountsPerSecond = 0;
    if (!xMapping || !yMapping || !mappingsShareDriver(*xMapping, *yMapping)
        || !toCounts(xPosition, xMapping->countsPerUnit, xPositionCounts)
        || !toCounts(yPosition, yMapping->countsPerUnit, yPositionCounts)
        || !toCounts(cruiseVelocity, xMapping->countsPerUnit, cruiseVelocityCountsPerSecond)
        || !toCounts(endVelocity, xMapping->countsPerUnit, endVelocityCountsPerSecond)) {
        return false;
    }

    return xMapping->driver->cncLinearAbsoluteXY(
        xPositionCounts,
        yPositionCounts,
        cruiseVelocityCountsPerSecond,
        endVelocityCountsPerSecond);
}

std::optional<AxisManager::AxisMapping> AxisManager::mappingFor(int logicalAxis) const
{
    std::scoped_lock lock(mappingsMutex_);
    const auto mapping = axisMappings_.find(logicalAxis);
    if (mapping == axisMappings_.end()) {
        return std::nullopt;
    }
    return mapping->second;
}

bool AxisManager::toCounts(double value, double countsPerUnit, int& counts)
{
    if (!std::isfinite(value) || !std::isfinite(countsPerUnit) || countsPerUnit <= 0.0) {
        return false;
    }

    const double scaledValue = value * countsPerUnit;
    if (scaledValue < static_cast<double>(std::numeric_limits<int>::min())
        || scaledValue > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }

    counts = static_cast<int>(std::llround(scaledValue));
    return true;
}

double AxisManager::fromCounts(int counts, double countsPerUnit)
{
    return static_cast<double>(counts) / countsPerUnit;
}

bool AxisManager::mappingsShareDriver(const AxisMapping& first, const AxisMapping& second)
{
    return first.driver && first.driver == second.driver;
}

} // namespace otms::device
