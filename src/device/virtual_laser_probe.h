#pragma once

#include "laser_probe.h"

namespace otms::device {

class VirtualLaserProbe final : public ILaserProbe
{
public:
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
    [[nodiscard]] LaserStatus requireConnection(const QString& operation) const;
    [[nodiscard]] static LaserStatus success(const QString& operation);
    [[nodiscard]] static LaserStatus failure(long code, const QString& message);
    [[nodiscard]] static bool validOutput(LaserOutput output);
    LaserStatus captureMeasurement(LaserOutput output, LaserMeasurement& measurement);

    LaserProbeConfig config_;
    bool connected_{};
    bool laserEnabled_{};
    bool measuring_{};
    std::uint8_t programNumber_{};
    std::uint32_t triggerCount_{};
    double zeroOffsetMicrometers_{};
};

} // namespace otms::device
