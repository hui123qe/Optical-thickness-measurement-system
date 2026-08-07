#include "cl3000_laser_driver.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <CL3_IF.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <thread>

#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(cl3000Log, "otms.device.cl3000")

namespace otms::device {
namespace {

constexpr long InvalidArgumentCode = -1;
constexpr long NotConnectedCode = -2;
constexpr long InvalidMeasurementCode = -3;

bool isVendorConnectionFailure(long vendorCode)
{
    return vendorCode == CL3IF_RC_ERR_USB
        || vendorCode == CL3IF_RC_ERR_ETHERNET
        || vendorCode == CL3IF_RC_ERR_CONNECT
        || vendorCode == CL3IF_RC_ERR_TIMEOUT;
}

MeasurementQuality qualityFromVendor(BYTE valueInfo)
{
    switch (static_cast<CL3IF_VALUE_INFO>(valueInfo)) {
    case CL3IF_VALUE_INFO_VALID:
        return MeasurementQuality::Valid;
    case CL3IF_VALUE_INFO_JUDGMENTSTANDBY:
        return MeasurementQuality::JudgmentStandby;
    case CL3IF_VALUE_INFO_INVALID:
        return MeasurementQuality::Invalid;
    case CL3IF_VALUE_INFO_OVERDISPRANGE_P:
        return MeasurementQuality::OverRangePositive;
    case CL3IF_VALUE_INFO_OVERDISPRANGE_N:
        return MeasurementQuality::OverRangeNegative;
    }
    return MeasurementQuality::Invalid;
}

MeasurementJudgment judgmentFromVendor(BYTE judgeResult)
{
    switch (static_cast<CL3IF_JUDGE_RESULT>(judgeResult)) {
    case CL3IF_JUDGE_RESULT_HI:
        return MeasurementJudgment::High;
    case CL3IF_JUDGE_RESULT_GO:
        return MeasurementJudgment::Good;
    case CL3IF_JUDGE_RESULT_LO:
        return MeasurementJudgment::Low;
    }
    return MeasurementJudgment::Unknown;
}

QString vendorErrorMessage(long vendorCode)
{
    switch (vendorCode) {
    case CL3IF_RC_OK:
        return QStringLiteral("成功");
    case CL3IF_RC_ERR_STATE_ERROR:
        return QStringLiteral("控制器当前状态不允许该操作");
    case CL3IF_RC_ERR_PARAMETER_NUMBER_ERROR:
        return QStringLiteral("参数数量错误");
    case CL3IF_RC_ERR_PARAMETER_RANGE_ERROR:
        return QStringLiteral("参数超出范围");
    case CL3IF_RC_ERR_UNIQUE_ERROR1:
    case CL3IF_RC_ERR_UNIQUE_ERROR2:
    case CL3IF_RC_ERR_UNIQUE_ERROR3:
        return QStringLiteral("控制器返回设备专用错误");
    case CL3IF_RC_ERR_INITIALIZE:
        return QStringLiteral("通信库初始化失败");
    case CL3IF_RC_ERR_NOT_PARAM:
        return QStringLiteral("必要参数缺失");
    case CL3IF_RC_ERR_USB:
        return QStringLiteral("USB 通信错误");
    case CL3IF_RC_ERR_ETHERNET:
        return QStringLiteral("以太网通信错误");
    case CL3IF_RC_ERR_CONNECT:
        return QStringLiteral("控制器连接错误");
    case CL3IF_RC_ERR_TIMEOUT:
        return QStringLiteral("控制器通信或测量超时");
    case CL3IF_RC_ERR_CHECKSUM:
        return QStringLiteral("通信校验错误");
    case CL3IF_RC_ERR_LIMIT_CONTROL_ERROR:
        return QStringLiteral("控制器限制错误");
    case CL3IF_RC_ERR_UNKNOWN:
        return QStringLiteral("控制器返回未知错误");
    case InvalidArgumentCode:
        return QStringLiteral("驱动参数无效");
    case NotConnectedCode:
        return QStringLiteral("激光控制器尚未连接");
    case InvalidMeasurementCode:
        return QStringLiteral("当前测量值无效，不能设置零点");
    default:
        return QStringLiteral("未识别的错误码");
    }
}

} // namespace

Cl3000LaserDriver::~Cl3000LaserDriver()
{
    if (!connected_) {
        return;
    }

    qCInfo(cl3000Log) << "Stopping CL-3000 measurement during shutdown";
    CL3IF_MeasurementControl(deviceId_, FALSE);
    qCInfo(cl3000Log) << "Disabling CL-3000 laser during shutdown";
    CL3IF_LightControl(deviceId_, FALSE);
    qCInfo(cl3000Log) << "Closing CL-3000 communication during shutdown";
    CL3IF_CloseCommunication(deviceId_);
}

LaserStatus Cl3000LaserDriver::setConfig(const LaserProbeConfig& config)
{
    if (!validOutput(config.measurementOutput)
        || !std::isfinite(config.micrometersPerCount)
        || config.micrometersPerCount <= 0.0
        || config.measurementTimeout.count() <= 0
        || config.pollingInterval.count() <= 0
        || config.softwareTriggerPulseWidth.count() <= 0
        || config.pollingInterval > config.measurementTimeout) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("配置测量参数"));
    }

    config_ = config;
    return makeStatus(CL3IF_RC_OK, QStringLiteral("配置测量参数"));
}

LaserProbeConfig Cl3000LaserDriver::config() const
{
    return config_;
}

LaserStatus Cl3000LaserDriver::connectEthernet(const LaserEthernetConfig& config)
{
    if (connected_) {
        return makeStatus(CL3IF_RC_OK, QStringLiteral("连接 CL-3000"));
    }
    if (config.deviceId < 0 || config.deviceId >= CL3IF_MAX_DEVICE_COUNT
        || config.port == 0 || config.timeout.count() <= 0
        || config.timeout.count() > std::numeric_limits<DWORD>::max()) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("连接 CL-3000"));
    }

    CL3IF_ETHERNET_SETTING ethernetSetting{};
    std::copy(config.ip.begin(), config.ip.end(), ethernetSetting.ipAddress);
    ethernetSetting.portNo = config.port;

    qCInfo(cl3000Log) << "Connecting CL-3000 over Ethernet, device" << config.deviceId
                      << "port" << config.port << "timeoutMs" << config.timeout.count();
    const LONG returnCode = CL3IF_OpenEthernetCommunication(
        config.deviceId,
        &ethernetSetting,
        static_cast<DWORD>(config.timeout.count()));
    connected_ = returnCode == CL3IF_RC_OK;
    if (connected_) {
        deviceId_ = config.deviceId;
    }
    return makeStatus(returnCode, QStringLiteral("连接 CL-3000"));
}

LaserStatus Cl3000LaserDriver::connectUsb(int deviceId, std::chrono::milliseconds timeout)
{
    if (connected_) {
        return makeStatus(CL3IF_RC_OK, QStringLiteral("连接 CL-3000"));
    }
    if (deviceId < 0 || deviceId >= CL3IF_MAX_DEVICE_COUNT || timeout.count() <= 0
        || timeout.count() > std::numeric_limits<DWORD>::max()) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("连接 CL-3000"));
    }

    qCInfo(cl3000Log) << "Connecting CL-3000 over USB, device" << deviceId
                      << "timeoutMs" << timeout.count();
    const LONG returnCode = CL3IF_OpenUsbCommunication(
        deviceId,
        static_cast<DWORD>(timeout.count()));
    connected_ = returnCode == CL3IF_RC_OK;
    if (connected_) {
        deviceId_ = deviceId;
    }
    return makeStatus(returnCode, QStringLiteral("连接 CL-3000"));
}

LaserStatus Cl3000LaserDriver::disconnect()
{
    if (!connected_) {
        return makeStatus(CL3IF_RC_OK, QStringLiteral("断开 CL-3000"));
    }

    qCInfo(cl3000Log) << "Stopping CL-3000 measurement before disconnect";
    const LONG stopCode = CL3IF_MeasurementControl(deviceId_, FALSE);
    qCInfo(cl3000Log) << "Disabling CL-3000 laser before disconnect";
    const LONG lightCode = CL3IF_LightControl(deviceId_, FALSE);
    qCInfo(cl3000Log) << "Closing CL-3000 communication";
    const LONG closeCode = CL3IF_CloseCommunication(deviceId_);
    connected_ = false;

    if (closeCode != CL3IF_RC_OK) {
        return makeStatus(closeCode, QStringLiteral("断开 CL-3000"));
    }
    if (stopCode != CL3IF_RC_OK) {
        return makeStatus(stopCode, QStringLiteral("停止测量"));
    }
    if (lightCode != CL3IF_RC_OK) {
        return makeStatus(lightCode, QStringLiteral("关闭激光"));
    }
    return makeStatus(CL3IF_RC_OK, QStringLiteral("断开 CL-3000"));
}

bool Cl3000LaserDriver::isConnected() const noexcept
{
    return connected_;
}

LaserStatus Cl3000LaserDriver::enableLaser()
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    qCInfo(cl3000Log) << "Enabling CL-3000 laser";
    return makeVendorStatus(CL3IF_LightControl(deviceId_, TRUE), QStringLiteral("开启激光"));
}

LaserStatus Cl3000LaserDriver::disableLaser()
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    qCInfo(cl3000Log) << "Disabling CL-3000 laser";
    return makeVendorStatus(CL3IF_LightControl(deviceId_, FALSE), QStringLiteral("关闭激光"));
}

LaserStatus Cl3000LaserDriver::startMeasurement()
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    qCInfo(cl3000Log) << "Starting CL-3000 measurement";
    return makeVendorStatus(
        CL3IF_MeasurementControl(deviceId_, TRUE),
        QStringLiteral("启动测量"));
}

LaserStatus Cl3000LaserDriver::stopMeasurement()
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    qCInfo(cl3000Log) << "Stopping CL-3000 measurement";
    return makeVendorStatus(
        CL3IF_MeasurementControl(deviceId_, FALSE),
        QStringLiteral("停止测量"));
}

LaserStatus Cl3000LaserDriver::setZero(LaserOutput output)
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (!validOutput(output)) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("设置零点"));
    }

    LaserMeasurement referenceMeasurement;
    const LaserStatus measurementStatus = readMeasurementFrame(output, referenceMeasurement);
    if (!measurementStatus.ok()) {
        return measurementStatus;
    }
    if (referenceMeasurement.quality != MeasurementQuality::Valid) {
        return makeStatus(InvalidMeasurementCode, QStringLiteral("设置零点"));
    }

    qCWarning(cl3000Log) << "Setting CL-3000 auto-zero for output"
                         << static_cast<int>(output) + 1;
    return makeVendorStatus(
        CL3IF_AutoZeroSingle(deviceId_, static_cast<BYTE>(output), TRUE),
        QStringLiteral("设置零点"));
}

LaserStatus Cl3000LaserDriver::clearZero(LaserOutput output)
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (!validOutput(output)) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("清除零点"));
    }
    qCWarning(cl3000Log) << "Clearing CL-3000 auto-zero for output"
                         << static_cast<int>(output) + 1;
    return makeVendorStatus(
        CL3IF_AutoZeroSingle(deviceId_, static_cast<BYTE>(output), FALSE),
        QStringLiteral("清除零点"));
}

LaserStatus Cl3000LaserDriver::resetOutput(LaserOutput output)
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (!validOutput(output)) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("复位输出"));
    }
    qCInfo(cl3000Log) << "Resetting CL-3000 output" << static_cast<int>(output) + 1;
    return makeVendorStatus(
        CL3IF_ResetSingle(deviceId_, static_cast<BYTE>(output)),
        QStringLiteral("复位输出"));
}

LaserStatus Cl3000LaserDriver::selectProgram(std::uint8_t programNumber)
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (programNumber > 7) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("切换程序"));
    }
    qCInfo(cl3000Log) << "Switching CL-3000 program" << programNumber;
    return makeVendorStatus(
        CL3IF_SwitchProgram(deviceId_, programNumber),
        QStringLiteral("切换程序"));
}

LaserStatus Cl3000LaserDriver::currentProgram(std::uint8_t& programNumber)
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    BYTE currentProgramNumber = 0;
    const LONG returnCode = CL3IF_GetProgramNo(deviceId_, &currentProgramNumber);
    if (returnCode == CL3IF_RC_OK) {
        programNumber = currentProgramNumber;
    }
    return makeVendorStatus(returnCode, QStringLiteral("读取当前程序"));
}

LaserStatus Cl3000LaserDriver::readLatest(LaserOutput output, LaserMeasurement& measurement)
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    return readMeasurementFrame(output, measurement);
}

LaserStatus Cl3000LaserDriver::measureOnce(
    LaserOutput output,
    std::chrono::milliseconds timeout,
    LaserMeasurement& measurement)
{
    const LaserStatus connectionStatus = requireConnection();
    if (!connectionStatus.ok()) {
        return connectionStatus;
    }
    if (!validOutput(output) || timeout.count() <= 0) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("单点测量"));
    }

    LaserMeasurement baseline;
    LaserStatus status = readMeasurementFrame(output, baseline);
    if (!status.ok()) {
        return status;
    }

    qCInfo(cl3000Log) << "Triggering CL-3000 measurement on output"
                      << static_cast<int>(output) + 1
                      << "pulseWidthMs" << config_.softwareTriggerPulseWidth.count();
    const LONG timingOnCode =
        CL3IF_TimingSingle(deviceId_, static_cast<BYTE>(output), TRUE);
    if (timingOnCode != CL3IF_RC_OK) {
        return makeVendorStatus(timingOnCode, QStringLiteral("启动软件触发"));
    }

    std::this_thread::sleep_for(config_.softwareTriggerPulseWidth);
    const LONG timingOffCode =
        CL3IF_TimingSingle(deviceId_, static_cast<BYTE>(output), FALSE);
    if (timingOffCode != CL3IF_RC_OK) {
        qCCritical(cl3000Log) << "Failed to release CL-3000 software timing input"
                              << "output" << static_cast<int>(output) + 1
                              << "vendorCode" << timingOffCode;
        return makeVendorStatus(timingOffCode, QStringLiteral("结束软件触发"));
    }

    qCInfo(cl3000Log) << "Waiting for the triggered CL-3000 measurement"
                      << "output" << static_cast<int>(output) + 1
                      << "timeoutMs" << timeout.count();
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + timeout;
    const std::chrono::milliseconds pollingInterval = std::min(config_.pollingInterval, timeout);
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(pollingInterval);
        status = readMeasurementFrame(output, measurement);
        if (!status.ok()) {
            return status;
        }
        if (measurement.triggerCount != baseline.triggerCount
            && measurement.quality == MeasurementQuality::Valid) {
            return makeStatus(CL3IF_RC_OK, QStringLiteral("单点测量"));
        }
    }

    return makeStatus(CL3IF_RC_ERR_TIMEOUT, QStringLiteral("单点测量"));
}

LaserStatus Cl3000LaserDriver::requireConnection() const
{
    return connected_
        ? makeStatus(CL3IF_RC_OK, QStringLiteral("检查连接"))
        : makeStatus(NotConnectedCode, QStringLiteral("检查连接"));
}

LaserStatus Cl3000LaserDriver::makeStatus(long vendorCode, const QString& operation) const
{
    const QString detail = vendorErrorMessage(vendorCode);
    return LaserStatus{
        vendorCode,
        vendorCode == CL3IF_RC_OK
            ? QStringLiteral("%1成功").arg(operation)
            : QStringLiteral("%1失败：%2").arg(operation, detail)};
}

LaserStatus Cl3000LaserDriver::makeVendorStatus(long vendorCode, const QString& operation)
{
    LaserStatus status = makeStatus(vendorCode, operation);
    if (!connected_ || !isVendorConnectionFailure(vendorCode)) {
        return status;
    }

    qCWarning(cl3000Log) << "CL-3000 communication failed; invalidating connection"
                         << "operation" << operation
                         << "vendorCode" << vendorCode;
    const LONG closeCode = CL3IF_CloseCommunication(deviceId_);
    connected_ = false;
    qCInfo(cl3000Log) << "Closed CL-3000 communication after failure"
                      << "closeCode" << closeCode;
    status.message += QStringLiteral("，连接已断开");
    return status;
}

LaserStatus Cl3000LaserDriver::readMeasurementFrame(
    LaserOutput output,
    LaserMeasurement& measurement)
{
    if (!validOutput(output)) {
        return makeStatus(InvalidArgumentCode, QStringLiteral("读取测量值"));
    }

    CL3IF_MEASUREMENT_DATA vendorMeasurement{};
    const LONG returnCode = CL3IF_GetMeasurementData(deviceId_, &vendorMeasurement);
    if (returnCode != CL3IF_RC_OK) {
        return makeVendorStatus(returnCode, QStringLiteral("读取测量值"));
    }

    const int outputIndex = static_cast<int>(output);
    const CL3IF_OUTMEASUREMENT_DATA& outputData = vendorMeasurement.outMeasurementData[outputIndex];
    measurement.output = output;
    measurement.rawValue = outputData.measurementValue;
    measurement.valueMicrometers = static_cast<double>(outputData.measurementValue)
        * config_.micrometersPerCount;
    measurement.quality = qualityFromVendor(outputData.valueInfo);
    measurement.judgment = judgmentFromVendor(outputData.judgeResult);
    measurement.triggerCount = vendorMeasurement.addInfo.triggerCount;
    measurement.pulseCount = vendorMeasurement.addInfo.pulseCount;
    return makeStatus(CL3IF_RC_OK, QStringLiteral("读取测量值"));
}

bool Cl3000LaserDriver::validOutput(LaserOutput output)
{
    const int outputIndex = static_cast<int>(output);
    return outputIndex >= 0 && outputIndex < CL3IF_MAX_OUT_COUNT;
}

} // namespace otms::device
