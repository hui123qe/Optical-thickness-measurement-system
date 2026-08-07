#pragma once

#include "laser_probe.h"

namespace otms::device {

class Cl3000LaserDriver final : public ILaserProbe
{
public:
    Cl3000LaserDriver() = default;
    ~Cl3000LaserDriver() override;

    Cl3000LaserDriver(const Cl3000LaserDriver&) = delete;
    Cl3000LaserDriver& operator=(const Cl3000LaserDriver&) = delete;

    LaserStatus setConfig(const LaserProbeConfig& config) override;
    [[nodiscard]] LaserProbeConfig config() const override;

    LaserStatus connectEthernet(const LaserEthernetConfig& config) override;
    LaserStatus connectUsb(int deviceId, std::chrono::milliseconds timeout) override;
    LaserStatus disconnect() override;
    [[nodiscard]] bool isConnected() const noexcept override;

    LaserStatus enableLaser() override;
    LaserStatus disableLaser() override;
    LaserStatus startMeasurement() override;
    LaserStatus stopMeasurement() override;

    LaserStatus setZero(LaserOutput output) override;
    LaserStatus clearZero(LaserOutput output) override;
    LaserStatus resetOutput(LaserOutput output) override;

    LaserStatus selectProgram(std::uint8_t programNumber) override;
    LaserStatus currentProgram(std::uint8_t& programNumber) override;

    LaserStatus readLatest(LaserOutput output, LaserMeasurement& measurement) override;
    LaserStatus measureOnce(
        LaserOutput output,
        std::chrono::milliseconds timeout,
        LaserMeasurement& measurement) override;

private:
    LaserStatus requireConnection() const;
    LaserStatus makeStatus(long vendorCode, const QString& operation) const;
    LaserStatus makeVendorStatus(long vendorCode, const QString& operation);
    LaserStatus readMeasurementFrame(LaserOutput output, LaserMeasurement& measurement);
    static bool validOutput(LaserOutput output);

    LaserProbeConfig config_;
    int deviceId_{};
    bool connected_{};
};

} // namespace otms::device
