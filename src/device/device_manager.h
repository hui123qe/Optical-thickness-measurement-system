#pragma once

#include "device_runtime_settings.h"
#include "laser_probe.h"

#include <memory>

#include <QObject>
#include <QTimer>

namespace otms::device {

class DeviceManager final : public QObject
{
    Q_OBJECT

public:
    static DeviceManager& instance();

    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    LaserStatus configureLaserProbe(const LaserProbeConfig& config);
    [[nodiscard]] LaserProbeConfig laserProbeConfig() const;
    LaserStatus loadLaserConfiguration(std::uint8_t& programNumber);
    LaserStatus saveLaserConfiguration(
        std::uint8_t programNumber,
        const LaserProbeConfig& config);

    LaserStatus connectLaserEthernet();
    LaserStatus connectLaserUsb(int deviceId, std::chrono::milliseconds timeout);
    LaserStatus disconnectLaser();
    [[nodiscard]] bool isLaserConnected() const noexcept;
    [[nodiscard]] bool isLaserMeasuring() const noexcept;

    LaserStatus enableLaser();
    LaserStatus disableLaser();
    LaserStatus startLaserMeasurement();
    LaserStatus stopLaserMeasurement();
    LaserStatus setLaserZero(LaserOutput output);
    LaserStatus clearLaserZero(LaserOutput output);
    LaserStatus resetLaserOutput(LaserOutput output);
    LaserStatus selectLaserProgram(std::uint8_t programNumber);
    LaserStatus currentLaserProgram(std::uint8_t& programNumber);
    LaserStatus readLatestLaserMeasurement(LaserMeasurement& measurement);
    LaserStatus measureLaserOnce(LaserMeasurement& measurement);

signals:
    void laserConnectionChanged(bool connected, const QString& detail);
    void laserMeasurementStateChanged(bool measuring);
    void laserMeasurementUpdated(const LaserMeasurement& measurement);

private slots:
    void pollLaserMeasurement();

private:
    explicit DeviceManager(QObject* parent = nullptr);
    LaserStatus observeLaserCommand(const LaserStatus& status);
    void setLaserMeasurementState(bool measuring);

    std::unique_ptr<ILaserProbe> laserProbe_;
    LaserEthernetConfig laserEthernetConfig_;
    bool laserEthernetConfigured_{};
    bool reportedLaserConnected_{};
    bool laserMeasuring_{};
    QTimer laserMeasurementPollTimer_;
};

} // namespace otms::device
