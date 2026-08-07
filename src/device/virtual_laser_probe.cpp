#include "virtual_laser_probe.h"

#include <cmath>
#include <limits>

namespace otms::device {
namespace {

constexpr long InvalidArgumentCode = -1;
constexpr long NotConnectedCode = -2;
constexpr long NotMeasuringCode = -3;
constexpr double DefaultMeasurementMicrometers = 1000.0;

} // namespace

LaserStatus VirtualLaserProbe::setConfig(const LaserProbeConfig& config)
{
    if (!validOutput(config.measurementOutput)
        || !std::isfinite(config.micrometersPerCount)
        || config.micrometersPerCount <= 0.0
        || config.measurementTimeout.count() <= 0
        || config.pollingInterval.count() <= 0
        || config.softwareTriggerPulseWidth.count() <= 0
        || config.pollingInterval > config.measurementTimeout) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟激光探头测量参数无效"));
    }

    config_ = config;
    return success(QStringLiteral("配置虚拟激光探头"));
}

LaserProbeConfig VirtualLaserProbe::config() const
{
    return config_;
}

LaserStatus VirtualLaserProbe::connectEthernet(const LaserEthernetConfig& config)
{
    if (config.deviceId < 0 || config.port == 0 || config.timeout.count() <= 0) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟激光探头以太网连接参数无效"));
    }

    connected_ = true;
    return success(QStringLiteral("连接虚拟激光探头"));
}

LaserStatus VirtualLaserProbe::connectUsb(int deviceId, std::chrono::milliseconds timeout)
{
    if (deviceId < 0 || timeout.count() <= 0) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟激光探头 USB 连接参数无效"));
    }

    connected_ = true;
    return success(QStringLiteral("连接虚拟激光探头"));
}

LaserStatus VirtualLaserProbe::disconnect()
{
    measuring_ = false;
    laserEnabled_ = false;
    connected_ = false;
    return success(QStringLiteral("断开虚拟激光探头"));
}

bool VirtualLaserProbe::isConnected() const noexcept
{
    return connected_;
}

LaserStatus VirtualLaserProbe::enableLaser()
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("开启虚拟激光"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }

    laserEnabled_ = true;
    return success(QStringLiteral("开启虚拟激光"));
}

LaserStatus VirtualLaserProbe::disableLaser()
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("关闭虚拟激光"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }

    measuring_ = false;
    laserEnabled_ = false;
    return success(QStringLiteral("关闭虚拟激光"));
}

LaserStatus VirtualLaserProbe::startMeasurement()
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("启动虚拟测量"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }

    measuring_ = true;
    return success(QStringLiteral("启动虚拟测量"));
}

LaserStatus VirtualLaserProbe::stopMeasurement()
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("停止虚拟测量"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }

    measuring_ = false;
    return success(QStringLiteral("停止虚拟测量"));
}

LaserStatus VirtualLaserProbe::setZero(LaserOutput output)
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("设置虚拟零点"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (!validOutput(output)) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟激光输出编号无效"));
    }

    zeroOffsetMicrometers_ = DefaultMeasurementMicrometers;
    return success(QStringLiteral("设置虚拟零点"));
}

LaserStatus VirtualLaserProbe::clearZero(LaserOutput output)
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("清除虚拟零点"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (!validOutput(output)) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟激光输出编号无效"));
    }

    zeroOffsetMicrometers_ = 0.0;
    return success(QStringLiteral("清除虚拟零点"));
}

LaserStatus VirtualLaserProbe::resetOutput(LaserOutput output)
{
    if (!validOutput(output)) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟激光输出编号无效"));
    }
    return clearZero(output);
}

LaserStatus VirtualLaserProbe::selectProgram(std::uint8_t programNumber)
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("切换虚拟测量程序"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (programNumber > 7) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟测量程序编号必须为 0～7"));
    }

    programNumber_ = programNumber;
    return success(QStringLiteral("切换虚拟测量程序"));
}

LaserStatus VirtualLaserProbe::currentProgram(std::uint8_t& programNumber)
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("读取虚拟测量程序"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }

    programNumber = programNumber_;
    return success(QStringLiteral("读取虚拟测量程序"));
}

LaserStatus VirtualLaserProbe::readLatest(
    LaserOutput output,
    LaserMeasurement& measurement)
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("读取虚拟测量值"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (!measuring_) {
        return failure(NotMeasuringCode, QStringLiteral("虚拟激光探头尚未开始测量"));
    }
    return captureMeasurement(output, measurement);
}

LaserStatus VirtualLaserProbe::measureOnce(
    LaserOutput output,
    std::chrono::milliseconds timeout,
    LaserMeasurement& measurement)
{
    const LaserStatus connectionStatus = requireConnection(QStringLiteral("执行虚拟单点测量"));
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (timeout.count() <= 0) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟单点测量超时时间无效"));
    }
    return captureMeasurement(output, measurement);
}

LaserStatus VirtualLaserProbe::requireConnection(const QString& operation) const
{
    return connected_
        ? success(operation)
        : failure(NotConnectedCode, QStringLiteral("%1失败：虚拟激光探头尚未连接").arg(operation));
}

LaserStatus VirtualLaserProbe::success(const QString& operation)
{
    return LaserStatus{0, QStringLiteral("%1成功").arg(operation)};
}

LaserStatus VirtualLaserProbe::failure(long code, const QString& message)
{
    return LaserStatus{code, message};
}

bool VirtualLaserProbe::validOutput(LaserOutput output)
{
    const int outputIndex = static_cast<int>(output);
    return outputIndex >= static_cast<int>(LaserOutput::Out1)
        && outputIndex <= static_cast<int>(LaserOutput::Out8);
}

LaserStatus VirtualLaserProbe::captureMeasurement(
    LaserOutput output,
    LaserMeasurement& measurement)
{
    if (!validOutput(output)) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟激光输出编号无效"));
    }

    const double valueMicrometers = DefaultMeasurementMicrometers - zeroOffsetMicrometers_;
    const double rawValue = valueMicrometers / config_.micrometersPerCount;
    if (rawValue < static_cast<double>(std::numeric_limits<std::int32_t>::min())
        || rawValue > static_cast<double>(std::numeric_limits<std::int32_t>::max())) {
        return failure(InvalidArgumentCode, QStringLiteral("虚拟测量值超出原始数据范围"));
    }

    ++triggerCount_;
    measurement.output = output;
    measurement.rawValue = static_cast<std::int32_t>(std::llround(rawValue));
    measurement.valueMicrometers = valueMicrometers;
    measurement.quality = MeasurementQuality::Valid;
    measurement.judgment = MeasurementJudgment::Good;
    measurement.triggerCount = triggerCount_;
    measurement.pulseCount = static_cast<std::int32_t>(triggerCount_);
    return success(QStringLiteral("读取虚拟测量值"));
}

} // namespace otms::device
