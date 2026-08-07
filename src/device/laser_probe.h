#pragma once

#include <array>
#include <chrono>
#include <cstdint>

#include <QMetaType>
#include <QString>

namespace otms::device {

enum class LaserOutput : std::uint8_t
{
    Out1 = 0,
    Out2,
    Out3,
    Out4,
    Out5,
    Out6,
    Out7,
    Out8
};

enum class MeasurementQuality
{
    Valid,
    JudgmentStandby,
    Invalid,
    OverRangePositive,
    OverRangeNegative
};

enum class MeasurementJudgment
{
    Unknown,
    High,
    Good,
    Low
};

struct LaserStatus
{
    long vendorCode{};
    QString message;

    [[nodiscard]] bool ok() const noexcept
    {
        return vendorCode == 0;
    }
};

struct LaserEthernetConfig
{
    int deviceId{};
    std::array<std::uint8_t, 4> ip{};
    std::uint16_t port{24685};
    std::chrono::milliseconds timeout{3000};
};

struct LaserProbeConfig
{
    LaserOutput measurementOutput{LaserOutput::Out1};
    double micrometersPerCount{1.0};
    std::chrono::milliseconds measurementTimeout{1000};
    std::chrono::milliseconds pollingInterval{10};
    std::chrono::milliseconds softwareTriggerPulseWidth{10};
};

struct LaserMeasurement
{
    LaserOutput output{LaserOutput::Out1};
    std::int32_t rawValue{};
    double valueMicrometers{};
    MeasurementQuality quality{MeasurementQuality::Invalid};
    MeasurementJudgment judgment{MeasurementJudgment::Unknown};
    std::uint32_t triggerCount{};
    std::int32_t pulseCount{};
};

class ILaserProbe
{
public:
    virtual ~ILaserProbe() = default;

    virtual LaserStatus setConfig(const LaserProbeConfig& config) = 0;
    [[nodiscard]] virtual LaserProbeConfig config() const = 0;

    virtual LaserStatus connectEthernet(const LaserEthernetConfig& config) = 0;
    virtual LaserStatus connectUsb(int deviceId, std::chrono::milliseconds timeout) = 0;
    virtual LaserStatus disconnect() = 0;
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;

    virtual LaserStatus enableLaser() = 0;
    virtual LaserStatus disableLaser() = 0;
    virtual LaserStatus startMeasurement() = 0;
    virtual LaserStatus stopMeasurement() = 0;

    virtual LaserStatus setZero(LaserOutput output) = 0;
    virtual LaserStatus clearZero(LaserOutput output) = 0;
    virtual LaserStatus resetOutput(LaserOutput output) = 0;

    virtual LaserStatus selectProgram(std::uint8_t programNumber) = 0;
    virtual LaserStatus currentProgram(std::uint8_t& programNumber) = 0;

    virtual LaserStatus readLatest(LaserOutput output, LaserMeasurement& measurement) = 0;
    virtual LaserStatus measureOnce(
        LaserOutput output,
        std::chrono::milliseconds timeout,
        LaserMeasurement& measurement) = 0;
};

} // namespace otms::device

Q_DECLARE_METATYPE(otms::device::LaserMeasurement)
Q_DECLARE_METATYPE(otms::device::LaserStatus)
