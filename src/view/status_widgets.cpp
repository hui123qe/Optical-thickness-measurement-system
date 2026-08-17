#include "status_widgets.h"

#include "widget_factory.h"

#include <cmath>

#include <QDateTime>
#include <QEvent>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QLabel* createValueLabel()
{
    QLabel* value = new QLabel(QStringLiteral("--"));
    value->setProperty("role", "coordinate");
    value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    value->setMinimumWidth(64);
    return value;
}

void addMeasurementRow(
    QHBoxLayout& layout,
    const QString& name,
    const QString& unit,
    QLabel* value)
{
    QLabel* nameLabel = new QLabel(name);
    nameLabel->setProperty("role", "measurementName");
    QLabel* unitLabel = new QLabel(unit);
    unitLabel->setProperty("role", "measurementUnit");
    layout.addWidget(nameLabel);
    layout.addWidget(value);
    layout.addWidget(unitLabel);
}

QFrame* createMeasurementCard(const QString& title)
{
    QFrame* card = new QFrame;
    card->setProperty("role", "coordinateCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 8, 14, 8);
    cardLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel(title);
    titleLabel->setProperty("role", "coordinateTitle");
    cardLayout->addWidget(titleLabel);

    return card;
}

void updateStatusBadge(QLabel* badge, const QString& text, const QString& state)
{
    badge->setText(text);
    badge->setProperty("state", state);
    badge->style()->unpolish(badge);
    badge->style()->polish(badge);
}

} // namespace

TopStatusWidget::TopStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("topStatus"));
    setFixedHeight(112);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(24, 12, 24, 12);
    layout->setSpacing(16);

    QLabel* systemName = new QLabel(QStringLiteral("光学厚度测量系统"));
    systemName->setProperty("role", "systemName");
    layout->addWidget(systemName);
    layout->addStretch();

    QPushButton* unlockDoor = WidgetFactory::createPrimaryButton(QStringLiteral("开锁"));
    unlockDoor->setMinimumSize(72, 72);
    unlockDoor->setToolTip(QStringLiteral("发送门锁解锁命令"));
    connect(unlockDoor, &QPushButton::clicked, this, [this] {
        emit doorLockCommandRequested(false);
    });
    layout->addWidget(unlockDoor);

    QPushButton* lockDoor = WidgetFactory::createPrimaryButton(QStringLiteral("关锁"));
    lockDoor->setMinimumSize(72, 72);
    lockDoor->setToolTip(QStringLiteral("发送门锁锁定命令"));
    connect(lockDoor, &QPushButton::clicked, this, [this] {
        emit doorLockCommandRequested(true);
    });
    layout->addWidget(lockDoor);

    layout->addWidget(createMotorPositionCard());
    layout->addWidget(createWorkpiecePositionCard());
    layout->addWidget(createLaserMeasurementCard());

    QPushButton* reset = new QPushButton(QStringLiteral("复位"));
    reset->setMinimumSize(88, 72);
    reset->setToolTip(QStringLiteral("清除软件故障锁存并重新评估启动条件"));
    connect(reset, &QPushButton::clicked, this, &TopStatusWidget::resetRequested);
    layout->addWidget(reset);

    QPushButton* emergencyStop = WidgetFactory::createDangerButton(QStringLiteral("急停"));
    emergencyStop->setProperty("emergencyStop", true);
    emergencyStop->setMinimumSize(120, 72);
    emergencyStop->setToolTip(QStringLiteral("软件急停入口，不能替代物理急停与安全联锁"));
    connect(emergencyStop, &QPushButton::clicked, this, &TopStatusWidget::emergencyStopRequested);
    layout->addWidget(emergencyStop);
}

QFrame* TopStatusWidget::createMotorPositionCard()
{
    QFrame* card = createMeasurementCard(QStringLiteral("电机绝对坐标"));
    QVBoxLayout* cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    QHBoxLayout* valuesLayout = new QHBoxLayout;
    valuesLayout->setSpacing(6);
    motorXPosition_ = createValueLabel();
    motorYPosition_ = createValueLabel();
    motorZPosition_ = createValueLabel();
    addMeasurementRow(*valuesLayout, QStringLiteral("X"), QStringLiteral("mm"), motorXPosition_);
    valuesLayout->addSpacing(8);
    addMeasurementRow(*valuesLayout, QStringLiteral("Y"), QStringLiteral("mm"), motorYPosition_);
    valuesLayout->addSpacing(8);
    addMeasurementRow(*valuesLayout, QStringLiteral("Z"), QStringLiteral("mm"), motorZPosition_);
    cardLayout->addLayout(valuesLayout);
    return card;
}

QFrame* TopStatusWidget::createWorkpiecePositionCard()
{
    QFrame* card = createMeasurementCard(QStringLiteral("物料相对坐标"));
    QVBoxLayout* cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    QHBoxLayout* valuesLayout = new QHBoxLayout;
    valuesLayout->setSpacing(6);
    addMeasurementRow(
        *valuesLayout, QStringLiteral("xw"), QStringLiteral("mm"), createValueLabel());
    valuesLayout->addSpacing(8);
    addMeasurementRow(
        *valuesLayout, QStringLiteral("yw"), QStringLiteral("mm"), createValueLabel());
    cardLayout->addLayout(valuesLayout);
    return card;
}

QFrame* TopStatusWidget::createLaserMeasurementCard()
{
    QFrame* card = createMeasurementCard(QStringLiteral("激光传感器"));
    QVBoxLayout* cardLayout = qobject_cast<QVBoxLayout*>(card->layout());
    QHBoxLayout* valuesLayout = new QHBoxLayout;
    valuesLayout->setSpacing(6);
    laserMeasurement_ = createValueLabel();
    addMeasurementRow(
        *valuesLayout, QStringLiteral("测量值"), QStringLiteral("mm"), laserMeasurement_);
    cardLayout->addLayout(valuesLayout);
    return card;
}

void TopStatusWidget::setLaserMeasurementMillimeters(double measurementMillimeters)
{
    laserMeasurement_->setText(
        laserConnected_ && std::isfinite(measurementMillimeters)
            ? QString::number(measurementMillimeters, 'f', 3)
            : QStringLiteral("--"));
}

void TopStatusWidget::setMotionConnectionState(bool connected)
{
    motionConnected_ = connected;
    if (!motionConnected_) {
        motorXPosition_->setText(QStringLiteral("--"));
        motorYPosition_->setText(QStringLiteral("--"));
        motorZPosition_->setText(QStringLiteral("--"));
    }
}

void TopStatusWidget::setMotorPositionMillimeters(
    double xMillimeters,
    double yMillimeters,
    double zMillimeters)
{
    if (!motionConnected_
        || !std::isfinite(xMillimeters)
        || !std::isfinite(yMillimeters)
        || !std::isfinite(zMillimeters)) {
        motorXPosition_->setText(QStringLiteral("--"));
        motorYPosition_->setText(QStringLiteral("--"));
        motorZPosition_->setText(QStringLiteral("--"));
        return;
    }

    motorXPosition_->setText(QString::number(xMillimeters, 'f', 3));
    motorYPosition_->setText(QString::number(yMillimeters, 'f', 3));
    motorZPosition_->setText(QString::number(zMillimeters, 'f', 3));
}

void TopStatusWidget::setLaserConnectionState(bool connected)
{
    laserConnected_ = connected;
    if (!laserConnected_) {
        laserMeasurement_->setText(QStringLiteral("--"));
    }
}

NavigationWidget::NavigationWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("navigationFrame"));
    setFixedWidth(176);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 22, 12, 18);
    layout->setSpacing(12);

    QLabel* caption = new QLabel(QStringLiteral("功能导航"));
    caption->setProperty("role", "navCaption");
    layout->addWidget(caption);

    navigation_ = new QListWidget;
    navigation_->setObjectName(QStringLiteral("navigation"));
    navigation_->setFocusPolicy(Qt::NoFocus);
    navigation_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation_->addItems({
        QStringLiteral("主界面"),
        QStringLiteral("日志记录"),
        QStringLiteral("设备调试")});
    layout->addWidget(navigation_);
    layout->addStretch();

    QLabel* hint = new QLabel(QStringLiteral("安全约束由\n设备与业务层执行"));
    hint->setProperty("role", "muted");
    layout->addWidget(hint);

    connect(navigation_, &QListWidget::currentRowChanged, this, [this](int index) {
        if (index >= 0 && navigation_->item(index) != nullptr) {
            emit pageSelected(index);
        }
    });
}

void NavigationWidget::selectPage(int index)
{
    navigation_->setCurrentRow(index);
}

RightStatusWidget::RightStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("rightStatus"));
    setFixedWidth(252);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 22, 18, 18);
    layout->setSpacing(12);

    QLabel* title = new QLabel(QStringLiteral("系统状态"));
    title->setProperty("role", "panelTitle");
    layout->addWidget(title);

    QGroupBox* devices = new QGroupBox(QStringLiteral("设备连接"));
    QVBoxLayout* deviceLayout = new QVBoxLayout(devices);
    deviceLayout->setContentsMargins(6, 4, 6, 4);
    deviceLayout->setSpacing(4);

    motionConnectionArea_ = new QWidget;
    motionConnectionArea_->setProperty("deviceConnectionArea", true);
    motionConnectionArea_->setCursor(Qt::PointingHandCursor);
    motionConnectionArea_->setToolTip(QStringLiteral("点击打开运动控制器连接窗口"));
    QHBoxLayout* motionLayout = new QHBoxLayout(motionConnectionArea_);
    motionLayout->setContentsMargins(6, 5, 6, 5);
    motionLayout->addWidget(new QLabel(QStringLiteral("运动控制器")));
    motionLayout->addStretch();
    motionState_ = WidgetFactory::createStatusBadge(QStringLiteral("离线"), QStringLiteral("offline"));
    motionLayout->addWidget(motionState_);
    deviceLayout->addWidget(motionConnectionArea_);

    laserConnectionArea_ = new QWidget;
    laserConnectionArea_->setProperty("deviceConnectionArea", true);
    laserConnectionArea_->setCursor(Qt::PointingHandCursor);
    laserConnectionArea_->setToolTip(QStringLiteral("点击打开激光探头连接窗口"));
    QHBoxLayout* laserLayout = new QHBoxLayout(laserConnectionArea_);
    laserLayout->setContentsMargins(6, 5, 6, 5);
    laserLayout->addWidget(new QLabel(QStringLiteral("激光探头")));
    laserLayout->addStretch();
    probeState_ = WidgetFactory::createStatusBadge(QStringLiteral("离线"), QStringLiteral("offline"));
    laserLayout->addWidget(probeState_);
    deviceLayout->addWidget(laserConnectionArea_);
    layout->addWidget(devices);

    const auto makeAreaClickable = [this](QWidget* area) {
        area->installEventFilter(this);
        const QList<QWidget*> children = area->findChildren<QWidget*>();
        for (QWidget* child : children) {
            child->setCursor(Qt::PointingHandCursor);
            child->installEventFilter(this);
        }
    };
    makeAreaClickable(motionConnectionArea_);
    makeAreaClickable(laserConnectionArea_);

    QGroupBox* machine = new QGroupBox(QStringLiteral("机器状态"));
    QFormLayout* machineLayout = new QFormLayout(machine);
    machineState_ = WidgetFactory::createStatusBadge(
        QStringLiteral("未就绪"),
        QStringLiteral("waiting"));
    machineLayout->addRow(QStringLiteral("状态"), machineState_);
    runMode_ = WidgetFactory::createStatusBadge(
        QStringLiteral("手动"),
        QStringLiteral("waiting"));
    machineLayout->addRow(QStringLiteral("运行模式"), runMode_);
    layout->addWidget(machine);

    QGroupBox* safety = new QGroupBox(QStringLiteral("安全状态"));
    QFormLayout* safetyLayout = new QFormLayout(safety);
    doorLockState_ = WidgetFactory::createStatusBadge(
        QStringLiteral("未知"),
        QStringLiteral("offline"));
    safetyLayout->addRow(QStringLiteral("门锁"), doorLockState_);
    lightCurtainState_ = WidgetFactory::createStatusBadge(
        QStringLiteral("未知"),
        QStringLiteral("offline"));
    safetyLayout->addRow(QStringLiteral("光幕"), lightCurtainState_);
    layout->addWidget(safety);

    QGroupBox* alert = new QGroupBox(QStringLiteral("报警与提示"));
    QVBoxLayout* alertLayout = new QVBoxLayout(alert);
    QLabel* noAlert = new QLabel(QStringLiteral("当前无活动报警"));
    noAlert->setWordWrap(true);
    alertLayout->addWidget(noAlert);
    layout->addWidget(alert);
    layout->addStretch();
}

void RightStatusWidget::setMotionConnectionState(bool connected)
{
    updateStatusBadge(
        motionState_,
        connected ? QStringLiteral("在线") : QStringLiteral("离线"),
        connected ? QStringLiteral("ready") : QStringLiteral("offline"));
}

bool RightStatusWidget::eventFilter(QObject* watched, QEvent* event)
{
    QWidget* watchedWidget = qobject_cast<QWidget*>(watched);
    if (watchedWidget != nullptr && event->type() == QEvent::MouseButtonRelease) {
        const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (watchedWidget == motionConnectionArea_
                || motionConnectionArea_->isAncestorOf(watchedWidget)) {
                emit motionConnectionRequested();
                return true;
            }
            if (watchedWidget == laserConnectionArea_
                || laserConnectionArea_->isAncestorOf(watchedWidget)) {
                emit laserConnectionRequested();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void RightStatusWidget::setProbeConnectionState(bool connected)
{
    updateStatusBadge(
        probeState_,
        connected ? QStringLiteral("在线") : QStringLiteral("离线"),
        connected ? QStringLiteral("ready") : QStringLiteral("offline"));
}

void RightStatusWidget::setDoorLockState(bool available, bool locked)
{
    updateStatusBadge(
        doorLockState_,
        !available
            ? QStringLiteral("未知")
            : locked ? QStringLiteral("已锁定") : QStringLiteral("已解锁"),
        !available
            ? QStringLiteral("offline")
            : locked ? QStringLiteral("ready") : QStringLiteral("waiting"));
}

void RightStatusWidget::setLightCurtainState(bool available, bool clear)
{
    updateStatusBadge(
        lightCurtainState_,
        !available
            ? QStringLiteral("未知")
            : clear ? QStringLiteral("安全") : QStringLiteral("遮挡"),
        !available
            ? QStringLiteral("offline")
            : clear ? QStringLiteral("ready") : QStringLiteral("terminated"));
}

void RightStatusWidget::setMachineState(
    otms::workflow::MachineState state,
    otms::workflow::RunMode mode)
{
    QString stateText;
    QString stateStyle;
    switch (state) {
    case otms::workflow::MachineState::NotReady:
        stateText = QStringLiteral("未就绪");
        stateStyle = QStringLiteral("waiting");
        break;
    case otms::workflow::MachineState::Ready:
        stateText = QStringLiteral("就绪");
        stateStyle = QStringLiteral("ready");
        break;
    case otms::workflow::MachineState::Running:
        stateText = QStringLiteral("运行中");
        stateStyle = QStringLiteral("ready");
        break;
    case otms::workflow::MachineState::Fault:
        stateText = QStringLiteral("故障");
        stateStyle = QStringLiteral("terminated");
        break;
    }
    updateStatusBadge(machineState_, stateText, stateStyle);
    updateStatusBadge(
        runMode_,
        mode == otms::workflow::RunMode::Manual
            ? QStringLiteral("手动")
            : QStringLiteral("自动"),
        mode == otms::workflow::RunMode::Manual
            ? QStringLiteral("waiting")
            : QStringLiteral("ready"));
}

BottomStatusBar::BottomStatusBar(QWidget* parent)
    : QStatusBar(parent)
{
    setSizeGripEnabled(false);
    addWidget(new QLabel(QStringLiteral("OPTICAL METROLOGY")));
    addWidget(new QLabel(QStringLiteral("版本 0.1.0")));

    QLabel* summary = new QLabel(QStringLiteral("系统就绪 · 界面原型模式"));
    summary->setProperty("role", "muted");
    addWidget(summary, 1);

    QLabel* clock = new QLabel;
    clock->setMinimumWidth(180);
    clock->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    addPermanentWidget(clock);

    QTimer* timer = new QTimer(clock);
    connect(timer, &QTimer::timeout, clock, [clock] {
        clock->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd  HH:mm:ss")));
    });
    timer->start(1000);
    clock->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd  HH:mm:ss")));
}
