#include "motion_controller_manager.h"

#include "../device/device_runtime_settings.h"
#include "virtual_motion_driver.h"

#include <cmath>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(motionControllerManagerLog, "otms.device.motion.manager")

namespace otms::device {
namespace {

struct ControllerConfig
{
    QString id;
    QString ipAddress;
    AgtioControllerType type{AgtioControllerType::Agc301};
};

struct AxisConfig
{
    LogicalAxis logicalAxis{};
    QString controllerId;
    int controllerAxis{};
    double countsPerUnit{1.0};
};

struct IoConfig
{
    QString logicalPoint;
    QString controllerId;
    int controllerPoint{};
    bool activeHigh{true};
};

struct MotionConfiguration
{
    std::vector<ControllerConfig> controllers;
    std::vector<AxisConfig> axes;
    std::vector<IoConfig> digitalInputs;
    std::vector<IoConfig> digitalOutputs;
};

void setError(QString* errorMessage, const QString& detail)
{
    if (errorMessage) {
        *errorMessage = detail;
    }
}

std::optional<AgtioControllerType> controllerTypeFromString(const QString& value)
{
    if (value.compare(QStringLiteral("Agc301"), Qt::CaseInsensitive) == 0) {
        return AgtioControllerType::Agc301;
    }
    if (value.compare(QStringLiteral("Agd155"), Qt::CaseInsensitive) == 0) {
        return AgtioControllerType::Agd155;
    }
    if (value.compare(QStringLiteral("Agc300"), Qt::CaseInsensitive) == 0) {
        return AgtioControllerType::Agc300;
    }
    if (value.compare(QStringLiteral("Agm800"), Qt::CaseInsensitive) == 0) {
        return AgtioControllerType::Agm800;
    }
    return std::nullopt;
}

std::optional<LogicalAxis> logicalAxisFromString(const QString& value)
{
    if (value.compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0) {
        return LogicalAxis::X;
    }
    if (value.compare(QStringLiteral("y"), Qt::CaseInsensitive) == 0) {
        return LogicalAxis::Y;
    }
    if (value.compare(QStringLiteral("z"), Qt::CaseInsensitive) == 0) {
        return LogicalAxis::Z;
    }
    return std::nullopt;
}

QString ioIdentifier(const QJsonValue& value)
{
    if (value.isString()) {
        return value.toString().trimmed();
    }
    if (value.isDouble()) {
        const int identifier = value.toInt(-1);
        return identifier >= 0 ? QString::number(identifier) : QString();
    }
    return {};
}

bool readIoMappings(
    const QJsonObject& motion,
    const QString& propertyName,
    std::vector<IoConfig>& mappings,
    QString* errorMessage)
{
    const QJsonValue value = motion.value(propertyName);
    if (value.isUndefined()) {
        return true;
    }
    if (!value.isArray()) {
        setError(errorMessage, QStringLiteral("motion.%1 must be an array").arg(propertyName));
        return false;
    }

    for (const QJsonValue& item : value.toArray()) {
        if (!item.isObject()) {
            setError(errorMessage, QStringLiteral("motion.%1 entries must be objects").arg(propertyName));
            return false;
        }
        const QJsonObject object = item.toObject();
        const QString logicalPoint = ioIdentifier(object.value(QStringLiteral("id")));
        const QString controllerId = object.value(QStringLiteral("controller")).toString().trimmed();
        const int controllerPoint = object.value(QStringLiteral("controllerPoint")).toInt(-1);
        const QJsonValue activeHighValue = object.value(QStringLiteral("activeHigh"));
        if (logicalPoint.isEmpty() || controllerId.isEmpty() || controllerPoint < 0
            || (!activeHighValue.isUndefined() && !activeHighValue.isBool())) {
            setError(errorMessage, QStringLiteral("motion.%1 contains an invalid mapping").arg(propertyName));
            return false;
        }
        mappings.push_back(IoConfig{
            logicalPoint,
            controllerId,
            controllerPoint,
            activeHighValue.toBool(true)});
    }
    return true;
}

bool loadMotionConfiguration(MotionConfiguration& config, QString* errorMessage)
{
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("device-connections.json"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("Cannot open %1: %2").arg(path, file.errorString()));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QStringLiteral("Invalid JSON in %1: %2").arg(path, parseError.errorString()));
        return false;
    }

    const QJsonValue motionValue = document.object().value(QStringLiteral("motion"));
    if (!motionValue.isObject()) {
        if (g_useVirtualMotionDriver) {
            qCWarning(motionControllerManagerLog)
                << "No motion configuration found; using backward-compatible virtual X/Y/Z mappings";
            config.controllers.push_back(ControllerConfig{
                QStringLiteral("stage"),
                QString(),
                AgtioControllerType::Agc301});
            config.axes.push_back(AxisConfig{
                LogicalAxis::X,
                QStringLiteral("stage"),
                0,
                1000.0});
            config.axes.push_back(AxisConfig{
                LogicalAxis::Y,
                QStringLiteral("stage"),
                1,
                1000.0});
            config.axes.push_back(AxisConfig{
                LogicalAxis::Z,
                QStringLiteral("stage"),
                2,
                1000.0});
            return true;
        }
        setError(errorMessage, QStringLiteral("Missing motion configuration in %1").arg(path));
        return false;
    }
    const QJsonObject motion = motionValue.toObject();

    const QJsonValue controllersValue = motion.value(QStringLiteral("controllers"));
    if (!controllersValue.isArray() || controllersValue.toArray().isEmpty()) {
        setError(errorMessage, QStringLiteral("motion.controllers must contain at least one controller"));
        return false;
    }
    for (const QJsonValue& item : controllersValue.toArray()) {
        if (!item.isObject()) {
            setError(errorMessage, QStringLiteral("motion.controllers entries must be objects"));
            return false;
        }
        const QJsonObject object = item.toObject();
        const QString id = object.value(QStringLiteral("id")).toString().trimmed();
        const QString ipAddress = object.value(QStringLiteral("ipAddress")).toString().trimmed();
        const auto type = controllerTypeFromString(object.value(QStringLiteral("type")).toString());
        if (id.isEmpty() || !type || (!g_useVirtualMotionDriver && ipAddress.isEmpty())) {
            setError(errorMessage, QStringLiteral("motion.controllers contains an invalid controller"));
            return false;
        }
        config.controllers.push_back(ControllerConfig{id, ipAddress, *type});
    }

    const QJsonValue axesValue = motion.value(QStringLiteral("axes"));
    if (!axesValue.isArray() || axesValue.toArray().isEmpty()) {
        setError(errorMessage, QStringLiteral("motion.axes must contain at least one axis"));
        return false;
    }
    for (const QJsonValue& item : axesValue.toArray()) {
        if (!item.isObject()) {
            setError(errorMessage, QStringLiteral("motion.axes entries must be objects"));
            return false;
        }
        const QJsonObject object = item.toObject();
        const auto logicalAxis = logicalAxisFromString(object.value(QStringLiteral("id")).toString());
        const QString controllerId = object.value(QStringLiteral("controller")).toString().trimmed();
        const int controllerAxis = object.value(QStringLiteral("controllerAxis")).toInt(-1);
        const double countsPerUnit = object.value(QStringLiteral("countsPerUnit")).toDouble(0.0);
        if (!logicalAxis || controllerId.isEmpty() || controllerAxis < 0
            || !std::isfinite(countsPerUnit) || countsPerUnit <= 0.0) {
            setError(errorMessage, QStringLiteral("motion.axes contains an invalid axis mapping"));
            return false;
        }
        config.axes.push_back(AxisConfig{*logicalAxis, controllerId, controllerAxis, countsPerUnit});
    }

    return readIoMappings(
               motion,
               QStringLiteral("digitalInputs"),
               config.digitalInputs,
               errorMessage)
        && readIoMappings(
            motion,
            QStringLiteral("digitalOutputs"),
            config.digitalOutputs,
            errorMessage);
}

std::unique_ptr<IMotionController> createController(const ControllerConfig& config)
{
    if (g_useVirtualMotionDriver) {
        return std::make_unique<VirtualMotionController>();
    }
    return std::make_unique<AgtioMotionController>(config.ipAddress, config.type);
}

} // namespace

MotionControllerManager& MotionControllerManager::instance()
{
    static MotionControllerManager manager;
    return manager;
}

MotionControllerManager::~MotionControllerManager()
{
    shutdown();
}

bool MotionControllerManager::initialize(QString* errorMessage)
{
    MotionConfiguration config;
    if (!loadMotionConfiguration(config, errorMessage)) {
        return false;
    }

    {
        std::unique_lock registryLock(registryMutex_);
        if (initialized_) {
            setError(errorMessage, QStringLiteral("Motion controller manager is already initialized"));
            return false;
        }

        for (const ControllerConfig& controllerConfig : config.controllers) {
            auto entry = std::make_unique<ControllerEntry>();
            entry->controller = createController(controllerConfig);
            if (!entry->controller || controllers_.contains(controllerConfig.id)) {
                setError(errorMessage, QStringLiteral("Duplicate or invalid motion controller: %1").arg(controllerConfig.id));
                controllers_.clear();
                return false;
            }
            controllers_.emplace(controllerConfig.id, std::move(entry));
        }

        for (const AxisConfig& axis : config.axes) {
            if (!controllers_.contains(axis.controllerId)
                || axisMappings_.contains(axis.logicalAxis)) {
                setError(errorMessage, QStringLiteral("Invalid or duplicate logical axis mapping"));
                controllers_.clear();
                axisMappings_.clear();
                return false;
            }
            axisMappings_.emplace(
                axis.logicalAxis,
                AxisMapping{axis.controllerId, axis.controllerAxis, axis.countsPerUnit});
        }

        const auto addIoMappings = [this](
                                       const std::vector<IoConfig>& source,
                                       std::map<QString, IoMapping>& destination) {
            for (const IoConfig& io : source) {
                if (!controllers_.contains(io.controllerId) || destination.contains(io.logicalPoint)) {
                    return false;
                }
                destination.emplace(
                    io.logicalPoint,
                    IoMapping{io.controllerId, io.controllerPoint, io.activeHigh});
            }
            return true;
        };
        if (!addIoMappings(config.digitalInputs, digitalInputMappings_)
            || !addIoMappings(config.digitalOutputs, digitalOutputMappings_)) {
            setError(errorMessage, QStringLiteral("Invalid or duplicate motion IO mapping"));
            controllers_.clear();
            axisMappings_.clear();
            digitalInputMappings_.clear();
            digitalOutputMappings_.clear();
            return false;
        }
        initialized_ = true;
    }

    qCInfo(motionControllerManagerLog)
        << "Motion controller manager initialized"
        << "virtual" << g_useVirtualMotionDriver
        << "controllers" << config.controllers.size()
        << "axes" << config.axes.size();

    for (const ControllerConfig& controller : config.controllers) {
        if (connectController(controller.id) != 1) {
            setError(errorMessage, QStringLiteral("Cannot connect motion controller: %1").arg(controller.id));
            shutdown();
            return false;
        }
    }

    if (!g_useVirtualMotionDriver) {
        return true;
    }

    for (const AxisConfig& axis : config.axes) {
        if (enableAxis(axis.logicalAxis) != 1) {
            setError(errorMessage, QStringLiteral("Cannot enable virtual axis"));
            shutdown();
            return false;
        }
    }
    return true;
}

void MotionControllerManager::shutdown()
{
    std::unique_lock registryLock(registryMutex_);
    for (auto& [controllerId, entry] : controllers_) {
        static_cast<void>(controllerId);
        std::scoped_lock commandLock(entry->commandMutex);
        entry->controller->disconnect();
    }
    controllers_.clear();
    axisMappings_.clear();
    digitalInputMappings_.clear();
    digitalOutputMappings_.clear();
    initialized_ = false;
}

bool MotionControllerManager::isInitialized() const
{
    std::shared_lock registryLock(registryMutex_);
    return initialized_;
}

QStringList MotionControllerManager::controllerIds() const
{
    std::shared_lock registryLock(registryMutex_);
    QStringList ids;
    ids.reserve(static_cast<qsizetype>(controllers_.size()));
    for (const auto& [controllerId, entry] : controllers_) {
        static_cast<void>(entry);
        ids.append(controllerId);
    }
    return ids;
}

MotionControllerManager::ControllerEntry* MotionControllerManager::controllerFor(
    const QString& controllerId) const
{
    const auto controller = controllers_.find(controllerId);
    return controller == controllers_.end() ? nullptr : controller->second.get();
}

const MotionControllerManager::AxisMapping* MotionControllerManager::axisMappingFor(
    LogicalAxis logicalAxis) const
{
    const auto mapping = axisMappings_.find(logicalAxis);
    return mapping == axisMappings_.end() ? nullptr : &mapping->second;
}

int MotionControllerManager::connectController(const QString& controllerId)
{
    std::shared_lock registryLock(registryMutex_);
    ControllerEntry* entry = controllerFor(controllerId);
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->connect();
}

int MotionControllerManager::disconnectController(const QString& controllerId)
{
    std::shared_lock registryLock(registryMutex_);
    ControllerEntry* entry = controllerFor(controllerId);
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->disconnect();
}

int MotionControllerManager::isControllerConnected(
    const QString& controllerId,
    int& connected) const
{
    std::shared_lock registryLock(registryMutex_);
    ControllerEntry* entry = controllerFor(controllerId);
    if (!entry) {
        connected = 0;
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->isConnected(connected);
}

int MotionControllerManager::isAxisControllerConnected(
    LogicalAxis logicalAxis,
    int& connected) const
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        connected = 0;
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->isConnected(connected);
}

int MotionControllerManager::enableAxis(LogicalAxis logicalAxis)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->enableAxis(mapping->controllerAxis);
}

int MotionControllerManager::disableAxis(LogicalAxis logicalAxis)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->disableAxis(mapping->controllerAxis);
}

int MotionControllerManager::isAxisEnabled(LogicalAxis logicalAxis, int& enabled) const
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->isAxisEnabled(mapping->controllerAxis, enabled);
}

int MotionControllerManager::homeAxis(
    LogicalAxis logicalAxis,
    bool asynchronous,
    int timeoutSeconds)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->homeAxis(mapping->controllerAxis, asynchronous, timeoutSeconds);
}

int MotionControllerManager::setSpeed(LogicalAxis logicalAxis, double unitsPerSecond)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry || !toCounts(unitsPerSecond, mapping->countsPerUnit, counts)) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->setSpeed(mapping->controllerAxis, counts);
}

int MotionControllerManager::jog(LogicalAxis logicalAxis, double unitsPerSecond)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry || !toCounts(unitsPerSecond, mapping->countsPerUnit, counts)) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->jog(mapping->controllerAxis, counts);
}

int MotionControllerManager::moveAbsolute(
    LogicalAxis logicalAxis,
    double position,
    bool asynchronous,
    int timeoutSeconds)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry || !toCounts(position, mapping->countsPerUnit, counts)) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->moveAbsolute(
        mapping->controllerAxis,
        counts,
        asynchronous,
        timeoutSeconds);
}

int MotionControllerManager::moveRelative(
    LogicalAxis logicalAxis,
    double distance,
    bool asynchronous,
    int timeoutSeconds)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry || !toCounts(distance, mapping->countsPerUnit, counts)) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->moveRelative(
        mapping->controllerAxis,
        counts,
        asynchronous,
        timeoutSeconds);
}

int MotionControllerManager::moveAbsoluteRepetitive(
    LogicalAxis logicalAxis,
    double position,
    int dwellTimeMs)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry || !toCounts(position, mapping->countsPerUnit, counts)) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->moveAbsoluteRepetitive(mapping->controllerAxis, counts, dwellTimeMs);
}

int MotionControllerManager::getPosition(LogicalAxis logicalAxis, double& position) const
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    const int result = entry->controller->getPosition(mapping->controllerAxis, counts);
    if (result == 1) {
        position = fromCounts(counts, mapping->countsPerUnit);
    }
    return result;
}

int MotionControllerManager::getReferencePosition(
    LogicalAxis logicalAxis,
    double& position) const
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    const int result = entry->controller->getReferencePosition(mapping->controllerAxis, counts);
    if (result == 1) {
        position = fromCounts(counts, mapping->countsPerUnit);
    }
    return result;
}

int MotionControllerManager::getVelocity(
    LogicalAxis logicalAxis,
    double& unitsPerSecond) const
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    int counts = 0;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    const int result = entry->controller->getVelocity(mapping->controllerAxis, counts);
    if (result == 1) {
        unitsPerSecond = fromCounts(counts, mapping->countsPerUnit);
    }
    return result;
}

int MotionControllerManager::getMotionStatus(LogicalAxis logicalAxis, int& status) const
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->getMotionStatus(mapping->controllerAxis, status);
}

int MotionControllerManager::stopAxis(
    LogicalAxis logicalAxis,
    bool asynchronous,
    int timeoutSeconds)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->stopAxis(mapping->controllerAxis, asynchronous, timeoutSeconds);
}

int MotionControllerManager::abortAxis(LogicalAxis logicalAxis)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->abortAxis(mapping->controllerAxis);
}

int MotionControllerManager::readDigitalInput(int logicalInput, bool& value) const
{
    return readDigitalInput(QString::number(logicalInput), value);
}

int MotionControllerManager::readDigitalInput(const QString& logicalInput, bool& value) const
{
    std::shared_lock registryLock(registryMutex_);
    const auto mapping = digitalInputMappings_.find(logicalInput);
    ControllerEntry* entry = mapping == digitalInputMappings_.end()
        ? nullptr
        : controllerFor(mapping->second.controllerId);
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    bool rawValue = false;
    const int result = entry->controller->readDigitalInput(
        mapping->second.controllerPoint,
        rawValue);
    if (result == 1) {
        value = rawValue == mapping->second.activeHigh;
    }
    return result;
}

int MotionControllerManager::readDigitalOutput(int logicalOutput, bool& value) const
{
    return readDigitalOutput(QString::number(logicalOutput), value);
}

int MotionControllerManager::readDigitalOutput(const QString& logicalOutput, bool& value) const
{
    std::shared_lock registryLock(registryMutex_);
    const auto mapping = digitalOutputMappings_.find(logicalOutput);
    ControllerEntry* entry = mapping == digitalOutputMappings_.end()
        ? nullptr
        : controllerFor(mapping->second.controllerId);
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    bool rawValue = false;
    const int result = entry->controller->readDigitalOutput(
        mapping->second.controllerPoint,
        rawValue);
    if (result == 1) {
        value = rawValue == mapping->second.activeHigh;
    }
    return result;
}

int MotionControllerManager::writeDigitalOutput(int logicalOutput, bool value)
{
    return writeDigitalOutput(QString::number(logicalOutput), value);
}

int MotionControllerManager::writeDigitalOutput(const QString& logicalOutput, bool value)
{
    std::shared_lock registryLock(registryMutex_);
    const auto mapping = digitalOutputMappings_.find(logicalOutput);
    ControllerEntry* entry = mapping == digitalOutputMappings_.end()
        ? nullptr
        : controllerFor(mapping->second.controllerId);
    if (!entry) {
        return 0;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->writeDigitalOutput(
        mapping->second.controllerPoint,
        value == mapping->second.activeHigh);
}

int MotionControllerManager::readDoorLocked(bool& locked) const
{
    return readDigitalInput(QStringLiteral("doorLocked"), locked);
}

int MotionControllerManager::readLightCurtainClear(bool& clear) const
{
    return readDigitalInput(QStringLiteral("lightCurtainClear"), clear);
}

int MotionControllerManager::setDoorLocked(bool locked)
{
    qCInfo(motionControllerManagerLog) << "Setting door lock" << locked;
    return writeDigitalOutput(QStringLiteral("doorLock"), locked);
}

int MotionControllerManager::setStackLamp(StackLampColor color, bool enabled)
{
    QString logicalOutput;
    switch (color) {
    case StackLampColor::Red:
        logicalOutput = QStringLiteral("redLamp");
        break;
    case StackLampColor::Yellow:
        logicalOutput = QStringLiteral("yellowLamp");
        break;
    case StackLampColor::Green:
        logicalOutput = QStringLiteral("greenLamp");
        break;
    }

    qCInfo(motionControllerManagerLog)
        << "Setting stack lamp" << logicalOutput << enabled;
    return writeDigitalOutput(logicalOutput, enabled);
}

bool MotionControllerManager::cncBegin(LogicalAxis logicalAxis)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return false;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->cncBegin();
}

bool MotionControllerManager::cncClearBuffer(LogicalAxis logicalAxis)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* mapping = axisMappingFor(logicalAxis);
    ControllerEntry* entry = mapping ? controllerFor(mapping->controllerId) : nullptr;
    if (!entry) {
        return false;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->cncClearBuffer();
}

bool MotionControllerManager::cncLinearAbsoluteXT(
    LogicalAxis logicalXAxis,
    LogicalAxis logicalTAxis,
    double xPosition,
    double tPosition,
    double cruiseVelocity,
    double endVelocity)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* xMapping = axisMappingFor(logicalXAxis);
    const AxisMapping* tMapping = axisMappingFor(logicalTAxis);
    if (!xMapping || !tMapping || xMapping->controllerId != tMapping->controllerId) {
        return false;
    }
    ControllerEntry* entry = controllerFor(xMapping->controllerId);
    int xCounts = 0;
    int tCounts = 0;
    int cruiseCounts = 0;
    int endCounts = 0;
    if (!entry || !toCounts(xPosition, xMapping->countsPerUnit, xCounts)
        || !toCounts(tPosition, tMapping->countsPerUnit, tCounts)
        || !toCounts(cruiseVelocity, xMapping->countsPerUnit, cruiseCounts)
        || !toCounts(endVelocity, xMapping->countsPerUnit, endCounts)) {
        return false;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->cncLinearAbsoluteXT(xCounts, tCounts, cruiseCounts, endCounts);
}

bool MotionControllerManager::cncLinearAbsoluteXY(
    LogicalAxis logicalXAxis,
    LogicalAxis logicalYAxis,
    double xPosition,
    double yPosition,
    double cruiseVelocity,
    double endVelocity)
{
    std::shared_lock registryLock(registryMutex_);
    const AxisMapping* xMapping = axisMappingFor(logicalXAxis);
    const AxisMapping* yMapping = axisMappingFor(logicalYAxis);
    if (!xMapping || !yMapping || xMapping->controllerId != yMapping->controllerId) {
        return false;
    }
    ControllerEntry* entry = controllerFor(xMapping->controllerId);
    int xCounts = 0;
    int yCounts = 0;
    int cruiseCounts = 0;
    int endCounts = 0;
    if (!entry || !toCounts(xPosition, xMapping->countsPerUnit, xCounts)
        || !toCounts(yPosition, yMapping->countsPerUnit, yCounts)
        || !toCounts(cruiseVelocity, xMapping->countsPerUnit, cruiseCounts)
        || !toCounts(endVelocity, xMapping->countsPerUnit, endCounts)) {
        return false;
    }
    std::scoped_lock commandLock(entry->commandMutex);
    return entry->controller->cncLinearAbsoluteXY(xCounts, yCounts, cruiseCounts, endCounts);
}

bool MotionControllerManager::toCounts(double value, double countsPerUnit, int& counts)
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

double MotionControllerManager::fromCounts(int counts, double countsPerUnit)
{
    return static_cast<double>(counts) / countsPerUnit;
}

} // namespace otms::device
