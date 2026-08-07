#include "device_manager.h"

#include "cl3000_laser_driver.h"
#include "virtual_laser_probe.h"

#include <cmath>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLoggingCategory>
#include <QSaveFile>
#include <QStringList>

Q_LOGGING_CATEGORY(deviceManagerLog, "otms.device.manager")

namespace otms::device {

bool g_useVirtualLaserProbe = true;

namespace {

constexpr long DeviceConfigurationErrorCode = -1001;

std::unique_ptr<ILaserProbe> createLaserProbe()
{
    if (g_useVirtualLaserProbe) {
        qCInfo(deviceManagerLog) << "Using virtual laser probe";
        return std::make_unique<VirtualLaserProbe>();
    }

    qCInfo(deviceManagerLog) << "Using CL-3000 laser probe";
    return std::make_unique<Cl3000LaserDriver>();
}

struct StoredLaserConfiguration
{
    LaserEthernetConfig ethernet;
    LaserProbeConfig probe;
    std::uint8_t programNumber{};
};

LaserStatus configurationError(const QString& detail)
{
    return LaserStatus{
        DeviceConfigurationErrorCode,
        QStringLiteral("设备配置错误：%1").arg(detail)};
}

bool readInteger(
    const QJsonObject& object,
    const QString& name,
    int minimum,
    int maximum,
    int& result)
{
    const QJsonValue value = object.value(name);
    if (!value.isDouble()) {
        return false;
    }

    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number
        || number < minimum || number > maximum) {
        return false;
    }

    result = static_cast<int>(number);
    return true;
}

bool readOptionalInteger(
    const QJsonObject& object,
    const QString& name,
    int minimum,
    int maximum,
    int& result)
{
    return !object.contains(name)
        || readInteger(object, name, minimum, maximum, result);
}

LaserStatus loadStoredLaserConfiguration(StoredLaserConfiguration& config)
{
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("device-connections.json"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return configurationError(
            QStringLiteral("无法打开 %1：%2").arg(path, file.errorString()));
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return configurationError(
            QStringLiteral("%1 不是有效的 JSON：%2")
                .arg(path, parseError.errorString()));
    }

    const QJsonValue cl3000Value = document.object().value(QStringLiteral("cl3000"));
    if (!cl3000Value.isObject()) {
        return configurationError(QStringLiteral("缺少 cl3000 对象"));
    }
    const QJsonObject cl3000 = cl3000Value.toObject();

    int deviceId = 0;
    int port = 0;
    int timeoutMs = 0;
    int programNumber = 0;
    int measurementOutput = 1;
    int measurementTimeoutMs = 1000;
    int pollingIntervalMs = 10;
    int softwareTriggerPulseWidthMs = 10;
    if (!readInteger(cl3000, QStringLiteral("deviceId"), 0, 2, deviceId)) {
        return configurationError(QStringLiteral("cl3000.deviceId 必须是 0～2 的整数"));
    }
    if (!readInteger(cl3000, QStringLiteral("port"), 1, 65535, port)) {
        return configurationError(QStringLiteral("cl3000.port 必须是 1～65535 的整数"));
    }
    if (!readInteger(
            cl3000,
            QStringLiteral("connectionTimeoutMs"),
            100,
            60000,
            timeoutMs)) {
        return configurationError(
            QStringLiteral("cl3000.connectionTimeoutMs 必须是 100～60000 的整数"));
    }
    if (!readOptionalInteger(
            cl3000,
            QStringLiteral("programNumber"),
            0,
            7,
            programNumber)) {
        return configurationError(QStringLiteral("cl3000.programNumber 必须是 0～7 的整数"));
    }
    if (!readOptionalInteger(
            cl3000,
            QStringLiteral("measurementOutput"),
            1,
            8,
            measurementOutput)) {
        return configurationError(
            QStringLiteral("cl3000.measurementOutput 必须是 1～8 的整数"));
    }
    if (!readOptionalInteger(
            cl3000,
            QStringLiteral("measurementTimeoutMs"),
            100,
            60000,
            measurementTimeoutMs)) {
        return configurationError(
            QStringLiteral("cl3000.measurementTimeoutMs 必须是 100～60000 的整数"));
    }
    if (!readOptionalInteger(
            cl3000,
            QStringLiteral("pollingIntervalMs"),
            1,
            1000,
            pollingIntervalMs)) {
        return configurationError(
            QStringLiteral("cl3000.pollingIntervalMs 必须是 1～1000 的整数"));
    }
    if (pollingIntervalMs > measurementTimeoutMs) {
        return configurationError(
            QStringLiteral("cl3000.pollingIntervalMs 不能大于 measurementTimeoutMs"));
    }
    if (!readOptionalInteger(
            cl3000,
            QStringLiteral("softwareTriggerPulseWidthMs"),
            1,
            60000,
            softwareTriggerPulseWidthMs)) {
        return configurationError(
            QStringLiteral("cl3000.softwareTriggerPulseWidthMs 必须是 1～60000 的整数"));
    }

    double micrometersPerCount = 0.001;
    const QJsonValue micrometersPerCountValue =
        cl3000.value(QStringLiteral("micrometersPerCount"));
    if (!micrometersPerCountValue.isUndefined()) {
        if (!micrometersPerCountValue.isDouble()
            || !std::isfinite(micrometersPerCountValue.toDouble())
            || micrometersPerCountValue.toDouble() <= 0.0) {
            return configurationError(
                QStringLiteral("cl3000.micrometersPerCount 必须是大于 0 的数值"));
        }
        micrometersPerCount = micrometersPerCountValue.toDouble();
    }

    const QJsonValue ipAddressValue = cl3000.value(QStringLiteral("ipAddress"));
    if (!ipAddressValue.isString()) {
        return configurationError(QStringLiteral("cl3000.ipAddress 必须是 IPv4 字符串"));
    }
    const QStringList segments =
        ipAddressValue.toString().trimmed().split('.', Qt::KeepEmptyParts);
    if (segments.size() != 4) {
        return configurationError(QStringLiteral("cl3000.ipAddress 必须包含四段 IPv4 地址"));
    }

    std::array<std::uint8_t, 4> ipAddress{};
    for (int index = 0; index < segments.size(); ++index) {
        bool converted = false;
        const int segment = segments.at(index).toInt(&converted);
        if (!converted || segment < 0 || segment > 255) {
            return configurationError(
                QStringLiteral("cl3000.ipAddress 的每一段必须是 0～255 的整数"));
        }
        ipAddress[static_cast<std::size_t>(index)] = static_cast<std::uint8_t>(segment);
    }

    config.ethernet = LaserEthernetConfig{
        deviceId,
        ipAddress,
        static_cast<std::uint16_t>(port),
        std::chrono::milliseconds(timeoutMs)};
    config.probe = LaserProbeConfig{
        static_cast<LaserOutput>(measurementOutput - 1),
        micrometersPerCount,
        std::chrono::milliseconds(measurementTimeoutMs),
        std::chrono::milliseconds(pollingIntervalMs),
        std::chrono::milliseconds(softwareTriggerPulseWidthMs)};
    config.programNumber = static_cast<std::uint8_t>(programNumber);
    qCInfo(deviceManagerLog) << "Loaded CL-3000 connection configuration from" << path;
    return LaserStatus{0, QStringLiteral("读取激光设备配置成功")};
}

QString formatIpAddress(const std::array<std::uint8_t, 4>& ipAddress)
{
    return QStringLiteral("%1.%2.%3.%4")
        .arg(ipAddress[0])
        .arg(ipAddress[1])
        .arg(ipAddress[2])
        .arg(ipAddress[3]);
}

LaserStatus saveStoredLaserConfiguration(const StoredLaserConfiguration& config)
{
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("device-connections.json"));
    QJsonObject cl3000;
    cl3000.insert(QStringLiteral("deviceId"), config.ethernet.deviceId);
    cl3000.insert(QStringLiteral("ipAddress"), formatIpAddress(config.ethernet.ip));
    cl3000.insert(QStringLiteral("port"), static_cast<int>(config.ethernet.port));
    cl3000.insert(
        QStringLiteral("connectionTimeoutMs"),
        static_cast<qint64>(config.ethernet.timeout.count()));
    cl3000.insert(QStringLiteral("programNumber"), static_cast<int>(config.programNumber));
    cl3000.insert(
        QStringLiteral("measurementOutput"),
        static_cast<int>(config.probe.measurementOutput) + 1);
    cl3000.insert(
        QStringLiteral("micrometersPerCount"),
        config.probe.micrometersPerCount);
    cl3000.insert(
        QStringLiteral("measurementTimeoutMs"),
        static_cast<qint64>(config.probe.measurementTimeout.count()));
    cl3000.insert(
        QStringLiteral("pollingIntervalMs"),
        static_cast<qint64>(config.probe.pollingInterval.count()));
    cl3000.insert(
        QStringLiteral("softwareTriggerPulseWidthMs"),
        static_cast<qint64>(config.probe.softwareTriggerPulseWidth.count()));

    QJsonObject root;
    root.insert(QStringLiteral("cl3000"), cl3000);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return configurationError(
            QStringLiteral("无法写入 %1：%2").arg(path, file.errorString()));
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        return configurationError(
            QStringLiteral("写入 %1 失败：%2").arg(path, file.errorString()));
    }
    if (!file.commit()) {
        return configurationError(
            QStringLiteral("保存 %1 失败：%2").arg(path, file.errorString()));
    }

    qCInfo(deviceManagerLog) << "Saved CL-3000 configuration to" << path;
    return LaserStatus{0, QStringLiteral("保存激光设备配置成功")};
}

} // namespace

DeviceManager& DeviceManager::instance()
{
    static DeviceManager manager;
    return manager;
}

DeviceManager::DeviceManager(QObject* parent)
    : QObject(parent)
    , laserProbe_(createLaserProbe())
{
}

LaserStatus DeviceManager::configureLaserProbe(const LaserProbeConfig& config)
{
    return laserProbe_->setConfig(config);
}

LaserProbeConfig DeviceManager::laserProbeConfig() const
{
    return laserProbe_->config();
}

LaserStatus DeviceManager::loadLaserConfiguration(std::uint8_t& programNumber)
{
    StoredLaserConfiguration stored;
    const LaserStatus loadStatus = loadStoredLaserConfiguration(stored);
    if (!loadStatus.ok()) {
        return loadStatus;
    }

    const LaserStatus probeStatus = laserProbe_->setConfig(stored.probe);
    if (!probeStatus.ok()) {
        return probeStatus;
    }

    laserEthernetConfig_ = stored.ethernet;
    laserEthernetConfigured_ = true;
    programNumber = stored.programNumber;
    return loadStatus;
}

LaserStatus DeviceManager::saveLaserConfiguration(
    std::uint8_t programNumber,
    const LaserProbeConfig& config)
{
    if (programNumber > 7) {
        return configurationError(QStringLiteral("程序号必须是 0～7 的整数"));
    }

    const LaserStatus probeStatus = laserProbe_->setConfig(config);
    if (!probeStatus.ok()) {
        return probeStatus;
    }

    StoredLaserConfiguration stored;
    const LaserStatus loadStatus = loadStoredLaserConfiguration(stored);
    if (!loadStatus.ok()) {
        return loadStatus;
    }
    stored.programNumber = programNumber;
    stored.probe = config;

    const LaserStatus saveStatus = saveStoredLaserConfiguration(stored);
    if (saveStatus.ok()) {
        laserEthernetConfig_ = stored.ethernet;
        laserEthernetConfigured_ = true;
    }
    return saveStatus;
}

LaserStatus DeviceManager::connectLaserEthernet()
{
    if (!laserEthernetConfigured_) {
        std::uint8_t programNumber = 0;
        const LaserStatus configurationStatus = loadLaserConfiguration(programNumber);
        if (!configurationStatus.ok()) {
            emit laserConnectionChanged(false, configurationStatus.message);
            return configurationStatus;
        }
    }

    const LaserStatus status = laserProbe_->connectEthernet(laserEthernetConfig_);
    reportedLaserConnected_ = laserProbe_->isConnected();
    emit laserConnectionChanged(reportedLaserConnected_, status.message);
    return status;
}

LaserStatus DeviceManager::connectLaserUsb(int deviceId, std::chrono::milliseconds timeout)
{
    const LaserStatus status = laserProbe_->connectUsb(deviceId, timeout);
    reportedLaserConnected_ = laserProbe_->isConnected();
    emit laserConnectionChanged(reportedLaserConnected_, status.message);
    return status;
}

LaserStatus DeviceManager::disconnectLaser()
{
    const LaserStatus status = laserProbe_->disconnect();
    reportedLaserConnected_ = laserProbe_->isConnected();
    emit laserConnectionChanged(reportedLaserConnected_, status.message);
    return status;
}

bool DeviceManager::isLaserConnected() const noexcept
{
    return laserProbe_->isConnected();
}

LaserStatus DeviceManager::observeLaserCommand(const LaserStatus& status)
{
    const bool connected = laserProbe_->isConnected();
    if (connected != reportedLaserConnected_) {
        reportedLaserConnected_ = connected;
        emit laserConnectionChanged(connected, status.message);
    }
    return status;
}

LaserStatus DeviceManager::enableLaser()
{
    return observeLaserCommand(laserProbe_->enableLaser());
}

LaserStatus DeviceManager::disableLaser()
{
    return observeLaserCommand(laserProbe_->disableLaser());
}

LaserStatus DeviceManager::startLaserMeasurement()
{
    return observeLaserCommand(laserProbe_->startMeasurement());
}

LaserStatus DeviceManager::stopLaserMeasurement()
{
    return observeLaserCommand(laserProbe_->stopMeasurement());
}

LaserStatus DeviceManager::setLaserZero(LaserOutput output)
{
    return observeLaserCommand(laserProbe_->setZero(output));
}

LaserStatus DeviceManager::clearLaserZero(LaserOutput output)
{
    return observeLaserCommand(laserProbe_->clearZero(output));
}

LaserStatus DeviceManager::resetLaserOutput(LaserOutput output)
{
    return observeLaserCommand(laserProbe_->resetOutput(output));
}

LaserStatus DeviceManager::selectLaserProgram(std::uint8_t programNumber)
{
    return observeLaserCommand(laserProbe_->selectProgram(programNumber));
}

LaserStatus DeviceManager::currentLaserProgram(std::uint8_t& programNumber)
{
    return observeLaserCommand(laserProbe_->currentProgram(programNumber));
}

LaserStatus DeviceManager::readLatestLaserMeasurement(LaserMeasurement& measurement)
{
    const LaserProbeConfig config = laserProbe_->config();
    return observeLaserCommand(
        laserProbe_->readLatest(config.measurementOutput, measurement));
}

LaserStatus DeviceManager::measureLaserOnce(LaserMeasurement& measurement)
{
    const LaserProbeConfig config = laserProbe_->config();
    return observeLaserCommand(
        laserProbe_->measureOnce(
            config.measurementOutput,
            config.measurementTimeout,
            measurement));
}

} // namespace otms::device
