#include "debug_page.h"

#include "../device/device_manager.h"
#include "widget_factory.h"

#include <chrono>

#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

QString qualityName(otms::device::MeasurementQuality quality)
{
    using otms::device::MeasurementQuality;
    switch (quality) {
    case MeasurementQuality::Valid:
        return QStringLiteral("有效");
    case MeasurementQuality::JudgmentStandby:
        return QStringLiteral("等待判定");
    case MeasurementQuality::Invalid:
        return QStringLiteral("无效");
    case MeasurementQuality::OverRangePositive:
        return QStringLiteral("正向超量程");
    case MeasurementQuality::OverRangeNegative:
        return QStringLiteral("负向超量程");
    }
    return QStringLiteral("未知");
}

QString judgmentName(otms::device::MeasurementJudgment judgment)
{
    using otms::device::MeasurementJudgment;
    switch (judgment) {
    case MeasurementJudgment::Unknown:
        return QStringLiteral("未知");
    case MeasurementJudgment::High:
        return QStringLiteral("HI");
    case MeasurementJudgment::Good:
        return QStringLiteral("GO");
    case MeasurementJudgment::Low:
        return QStringLiteral("LO");
    }
    return QStringLiteral("未知");
}

} // namespace

LaserDebugPage::LaserDebugPage(QWidget* parent)
    : QWidget(parent)
{
    using otms::device::DeviceManager;
    using otms::device::LaserStatus;

    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 20, 24, 20);
    pageLayout->setSpacing(14);
    pageLayout->addWidget(WidgetFactory::createSectionTitle(
        QStringLiteral("设备调试"),
        QStringLiteral("KEYENCE CL-3000 连接、调零与软件触发测量")));

    QHBoxLayout* settingsLayout = new QHBoxLayout;
    settingsLayout->setSpacing(14);

    QGroupBox* connectionGroup = new QGroupBox(QStringLiteral("设备连接"));
    QFormLayout* connectionLayout = new QFormLayout(connectionGroup);
    connectionState_ = WidgetFactory::createStatusBadge(QStringLiteral("离线"), QStringLiteral("offline"));
    QLabel* connectionHint = new QLabel(QStringLiteral("连接参数来自程序目录下的 device-connections.json。"));
    connectionHint->setWordWrap(true);
    connectionHint->setProperty("role", "muted");
    connectionLayout->addRow(connectionHint);
    connectionLayout->addRow(QStringLiteral("连接状态"), connectionState_);
    QHBoxLayout* connectionButtons = new QHBoxLayout;
    connectButton_ = WidgetFactory::createPrimaryButton(QStringLiteral("连接"));
    disconnectButton_ = new QPushButton(QStringLiteral("断开"));
    connectionButtons->addWidget(connectButton_);
    connectionButtons->addWidget(disconnectButton_);
    connectionLayout->addRow(connectionButtons);
    settingsLayout->addWidget(connectionGroup, 1);

    QGroupBox* programGroup = new QGroupBox(QStringLiteral("控制器程序"));
    QFormLayout* programLayout = new QFormLayout(programGroup);
    programInput_ = new QSpinBox;
    programInput_->setRange(0, 7);
    QPushButton* selectProgramButton = new QPushButton(QStringLiteral("切换程序"));
    programLayout->addRow(QStringLiteral("程序号"), programInput_);
    programLayout->addRow(selectProgramButton);
    settingsLayout->addWidget(programGroup, 1);

    QGroupBox* measurementConfigGroup = new QGroupBox(QStringLiteral("单点测量参数"));
    QFormLayout* measurementConfigLayout = new QFormLayout(measurementConfigGroup);
    outputInput_ = new QComboBox;
    for (int outputIndex = 0; outputIndex < 8; ++outputIndex) {
        outputInput_->addItem(QStringLiteral("OUT%1").arg(outputIndex + 1), outputIndex);
    }
    scaleInput_ = new QDoubleSpinBox;
    scaleInput_->setDecimals(6);
    scaleInput_->setRange(0.000001, 1000000.0);
    scaleInput_->setValue(0.001);
    scaleInput_->setSuffix(QStringLiteral(" μm/count"));
    measurementTimeoutInput_ = new QSpinBox;
    measurementTimeoutInput_->setRange(100, 60000);
    measurementTimeoutInput_->setValue(1000);
    measurementTimeoutInput_->setSuffix(QStringLiteral(" ms"));
    pollingIntervalInput_ = new QSpinBox;
    pollingIntervalInput_->setRange(1, 1000);
    pollingIntervalInput_->setValue(10);
    pollingIntervalInput_->setSuffix(QStringLiteral(" ms"));
    measurementConfigLayout->addRow(QStringLiteral("测量输出"), outputInput_);
    measurementConfigLayout->addRow(QStringLiteral("数值换算"), scaleInput_);
    measurementConfigLayout->addRow(QStringLiteral("测量超时"), measurementTimeoutInput_);
    measurementConfigLayout->addRow(QStringLiteral("轮询间隔"), pollingIntervalInput_);
    QLabel* scaleHint = new QLabel(QStringLiteral("换算系数必须与 CL-Navigator 当前程序的显示单位一致。"));
    scaleHint->setWordWrap(true);
    scaleHint->setProperty("role", "muted");
    measurementConfigLayout->addRow(scaleHint);
    QPushButton* saveConfigButton = new QPushButton(QStringLiteral("保存参数"));
    measurementConfigLayout->addRow(saveConfigButton);
    settingsLayout->addWidget(measurementConfigGroup, 1);
    pageLayout->addLayout(settingsLayout);

    QGroupBox* operationGroup = new QGroupBox(QStringLiteral("分步操作"));
    QGridLayout* operationLayout = new QGridLayout(operationGroup);
    QPushButton* enableLaserButton = new QPushButton(QStringLiteral("开启激光"));
    QPushButton* disableLaserButton = new QPushButton(QStringLiteral("关闭激光"));
    QPushButton* startMeasurementButton = new QPushButton(QStringLiteral("启动测量"));
    QPushButton* stopMeasurementButton = new QPushButton(QStringLiteral("停止测量"));
    QPushButton* setZeroButton = new QPushButton(QStringLiteral("设置零点"));
    QPushButton* clearZeroButton = new QPushButton(QStringLiteral("清除零点"));
    QPushButton* resetOutputButton = new QPushButton(QStringLiteral("复位输出"));
    QPushButton* readLatestButton = new QPushButton(QStringLiteral("读取当前值"));
    QPushButton* measureOnceButton = WidgetFactory::createPrimaryButton(QStringLiteral("软件触发测量"));
    operationLayout->addWidget(enableLaserButton, 0, 0);
    operationLayout->addWidget(disableLaserButton, 0, 1);
    operationLayout->addWidget(startMeasurementButton, 0, 2);
    operationLayout->addWidget(stopMeasurementButton, 0, 3);
    operationLayout->addWidget(setZeroButton, 1, 0);
    operationLayout->addWidget(clearZeroButton, 1, 1);
    operationLayout->addWidget(resetOutputButton, 1, 2);
    operationLayout->addWidget(readLatestButton, 1, 3);
    operationLayout->addWidget(measureOnceButton, 1, 4);
    connectedControls_ = {
        selectProgramButton,
        enableLaserButton,
        disableLaserButton,
        startMeasurementButton,
        stopMeasurementButton,
        setZeroButton,
        clearZeroButton,
        resetOutputButton,
        readLatestButton,
        measureOnceButton};
    pageLayout->addWidget(operationGroup);

    QHBoxLayout* resultLayout = new QHBoxLayout;
    QGroupBox* measurementResultGroup = new QGroupBox(QStringLiteral("测量结果"));
    QFormLayout* measurementResultLayout = new QFormLayout(measurementResultGroup);
    rawValue_ = new QLabel(QStringLiteral("--"));
    convertedValue_ = new QLabel(QStringLiteral("-- μm"));
    quality_ = new QLabel(QStringLiteral("--"));
    judgment_ = new QLabel(QStringLiteral("--"));
    triggerCount_ = new QLabel(QStringLiteral("--"));
    measurementResultLayout->addRow(QStringLiteral("原始值"), rawValue_);
    measurementResultLayout->addRow(QStringLiteral("换算值"), convertedValue_);
    measurementResultLayout->addRow(QStringLiteral("数据质量"), quality_);
    measurementResultLayout->addRow(QStringLiteral("判定"), judgment_);
    measurementResultLayout->addRow(QStringLiteral("触发计数"), triggerCount_);
    resultLayout->addWidget(measurementResultGroup, 1);

    QGroupBox* logGroup = new QGroupBox(QStringLiteral("操作日志"));
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    operationLog_ = new QTextEdit;
    operationLog_->setReadOnly(true);
    logLayout->addWidget(operationLog_);
    resultLayout->addWidget(logGroup, 2);
    pageLayout->addLayout(resultLayout, 1);

    DeviceManager& manager = DeviceManager::instance();
    connect(&manager, &DeviceManager::laserConnectionChanged,
        this, &LaserDebugPage::updateConnectionState);

    std::uint8_t configuredProgram = 0;
    const LaserStatus loadConfigurationStatus =
        manager.loadLaserConfiguration(configuredProgram);
    appendStatus(QStringLiteral("加载设备配置"), loadConfigurationStatus);
    if (loadConfigurationStatus.ok()) {
        const otms::device::LaserProbeConfig config = manager.laserProbeConfig();
        programInput_->setValue(configuredProgram);
        outputInput_->setCurrentIndex(static_cast<int>(config.measurementOutput));
        scaleInput_->setValue(config.micrometersPerCount);
        measurementTimeoutInput_->setValue(
            static_cast<int>(config.measurementTimeout.count()));
        pollingIntervalInput_->setValue(
            static_cast<int>(config.pollingInterval.count()));
    }

    connect(outputInput_, &QComboBox::currentIndexChanged,
        this, &LaserDebugPage::applyProbeConfigFromControl);
    connect(scaleInput_, &QDoubleSpinBox::valueChanged,
        this, &LaserDebugPage::applyProbeConfigFromControl);
    connect(measurementTimeoutInput_, &QSpinBox::valueChanged,
        this, &LaserDebugPage::applyProbeConfigFromControl);
    connect(pollingIntervalInput_, &QSpinBox::valueChanged,
        this, &LaserDebugPage::applyProbeConfigFromControl);
    connect(saveConfigButton, &QPushButton::clicked,
        this, &LaserDebugPage::saveProbeConfig);

    connect(connectButton_, &QPushButton::clicked,
        this, &LaserDebugPage::connectLaser);
    connect(disconnectButton_, &QPushButton::clicked,
        this, &LaserDebugPage::disconnectLaser);
    connect(selectProgramButton, &QPushButton::clicked,
        this, &LaserDebugPage::selectLaserProgram);
    connect(enableLaserButton, &QPushButton::clicked,
        this, &LaserDebugPage::enableLaser);
    connect(disableLaserButton, &QPushButton::clicked,
        this, &LaserDebugPage::disableLaser);
    connect(startMeasurementButton, &QPushButton::clicked,
        this, &LaserDebugPage::startLaserMeasurement);
    connect(stopMeasurementButton, &QPushButton::clicked,
        this, &LaserDebugPage::stopLaserMeasurement);
    connect(setZeroButton, &QPushButton::clicked,
        this, &LaserDebugPage::setLaserZero);
    connect(clearZeroButton, &QPushButton::clicked,
        this, &LaserDebugPage::clearLaserZero);
    connect(resetOutputButton, &QPushButton::clicked,
        this, &LaserDebugPage::resetLaserOutput);
    connect(readLatestButton, &QPushButton::clicked,
        this, &LaserDebugPage::readLatestLaserMeasurement);
    connect(measureOnceButton, &QPushButton::clicked,
        this, &LaserDebugPage::measureLaserOnce);

    updateConnectionState(manager.isLaserConnected(), QStringLiteral("尚未连接设备"));
}

void LaserDebugPage::applyProbeConfigFromControl()
{
    applyProbeConfig();
}

void LaserDebugPage::saveProbeConfig()
{
    if (!applyProbeConfig()) {
        return;
    }

    otms::device::DeviceManager& manager = otms::device::DeviceManager::instance();
    appendStatus(
        QStringLiteral("保存设备配置"),
        manager.saveLaserConfiguration(
            static_cast<std::uint8_t>(programInput_->value()),
            manager.laserProbeConfig()));
}

void LaserDebugPage::connectLaser()
{
    if (!applyProbeConfig()) {
        return;
    }

    appendStatus(
        QStringLiteral("连接"),
        otms::device::DeviceManager::instance().connectLaserEthernet());
}

void LaserDebugPage::disconnectLaser()
{
    appendStatus(
        QStringLiteral("断开"),
        otms::device::DeviceManager::instance().disconnectLaser());
}

void LaserDebugPage::selectLaserProgram()
{
    appendStatus(
        QStringLiteral("切换程序"),
        otms::device::DeviceManager::instance().selectLaserProgram(
            static_cast<std::uint8_t>(programInput_->value())));
}

void LaserDebugPage::enableLaser()
{
    if (confirmHardwareAction(
            QStringLiteral("确认开启激光"),
            QStringLiteral("即将开启 CL-3000 激光发射，请确认探头区域满足激光安全要求。"))) {
        appendStatus(
            QStringLiteral("开启激光"),
            otms::device::DeviceManager::instance().enableLaser());
    }
}

void LaserDebugPage::disableLaser()
{
    appendStatus(
        QStringLiteral("关闭激光"),
        otms::device::DeviceManager::instance().disableLaser());
}

void LaserDebugPage::startLaserMeasurement()
{
    appendStatus(
        QStringLiteral("启动测量"),
        otms::device::DeviceManager::instance().startLaserMeasurement());
}

void LaserDebugPage::stopLaserMeasurement()
{
    appendStatus(
        QStringLiteral("停止测量"),
        otms::device::DeviceManager::instance().stopLaserMeasurement());
}

void LaserDebugPage::setLaserZero()
{
    if (confirmHardwareAction(
            QStringLiteral("确认设置零点"),
            QStringLiteral("当前有效测量值将作为所选输出的零点。请确认基准面已正确就位。"))) {
        appendStatus(
            QStringLiteral("设置零点"),
            otms::device::DeviceManager::instance().setLaserZero(selectedOutput()));
    }
}

void LaserDebugPage::clearLaserZero()
{
    if (confirmHardwareAction(
            QStringLiteral("确认清除零点"),
            QStringLiteral("即将取消所选输出的自动调零值。"))) {
        appendStatus(
            QStringLiteral("清除零点"),
            otms::device::DeviceManager::instance().clearLaserZero(selectedOutput()));
    }
}

void LaserDebugPage::resetLaserOutput()
{
    appendStatus(
        QStringLiteral("复位输出"),
        otms::device::DeviceManager::instance().resetLaserOutput(selectedOutput()));
}

void LaserDebugPage::readLatestLaserMeasurement()
{
    if (!applyProbeConfig()) {
        return;
    }

    otms::device::LaserMeasurement measurement;
    const otms::device::LaserStatus status =
        otms::device::DeviceManager::instance().readLatestLaserMeasurement(measurement);
    appendStatus(QStringLiteral("读取当前值"), status);
    if (status.ok()) {
        showMeasurement(measurement);
    }
}

void LaserDebugPage::measureLaserOnce()
{
    if (!applyProbeConfig()) {
        return;
    }

    otms::device::LaserMeasurement measurement;
    const otms::device::LaserStatus status =
        otms::device::DeviceManager::instance().measureLaserOnce(measurement);
    appendStatus(QStringLiteral("软件触发测量"), status);
    if (status.ok()) {
        showMeasurement(measurement);
    }
}

bool LaserDebugPage::applyProbeConfig()
{
    const otms::device::LaserProbeConfig config{
        selectedOutput(),
        scaleInput_->value(),
        std::chrono::milliseconds(measurementTimeoutInput_->value()),
        std::chrono::milliseconds(pollingIntervalInput_->value())};
    const otms::device::LaserStatus status =
        otms::device::DeviceManager::instance().configureLaserProbe(config);
    if (!status.ok()) {
        appendStatus(QStringLiteral("配置测量参数"), status);
        return false;
    }
    return true;
}

otms::device::LaserOutput LaserDebugPage::selectedOutput() const
{
    return static_cast<otms::device::LaserOutput>(outputInput_->currentData().toInt());
}

void LaserDebugPage::appendStatus(
    const QString& operation,
    const otms::device::LaserStatus& status)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    operationLog_->append(QStringLiteral("[%1] %2：%3（code=%4）")
                              .arg(timestamp, operation, status.message)
                              .arg(status.vendorCode));
}

void LaserDebugPage::showMeasurement(const otms::device::LaserMeasurement& measurement)
{
    rawValue_->setText(QString::number(measurement.rawValue));
    convertedValue_->setText(QStringLiteral("%1 μm").arg(measurement.valueMicrometers, 0, 'f', 6));
    quality_->setText(qualityName(measurement.quality));
    judgment_->setText(judgmentName(measurement.judgment));
    triggerCount_->setText(QString::number(measurement.triggerCount));
}

void LaserDebugPage::updateConnectionState(bool connected, const QString& detail)
{
    connectionState_->setText(connected ? QStringLiteral("在线") : QStringLiteral("离线"));
    connectionState_->setProperty("state", connected ? QStringLiteral("ready") : QStringLiteral("offline"));
    connectionState_->style()->unpolish(connectionState_);
    connectionState_->style()->polish(connectionState_);
    appendStatus(QStringLiteral("连接状态"), otms::device::LaserStatus{
        connected ? 0L : -2L,
        detail});
    updateControls();
}

void LaserDebugPage::updateControls()
{
    const bool connected = otms::device::DeviceManager::instance().isLaserConnected();
    connectButton_->setEnabled(!connected);
    disconnectButton_->setEnabled(connected);
    for (QPushButton* control : connectedControls_) {
        control->setEnabled(connected);
    }
}

bool LaserDebugPage::confirmHardwareAction(const QString& title, const QString& detail)
{
    return QMessageBox::warning(
               this,
               title,
               detail,
               QMessageBox::Yes | QMessageBox::No,
               QMessageBox::No)
        == QMessageBox::Yes;
}
