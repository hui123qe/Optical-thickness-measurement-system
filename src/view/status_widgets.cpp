#include "status_widgets.h"

#include "widget_factory.h"

#include <cmath>

#include <QDateTime>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QFrame* createCoordinateCard(const QString& title, const QStringList& axes)
{
    QFrame* card = new QFrame;
    card->setProperty("role", "coordinateCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 8, 14, 8);
    cardLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel(title);
    titleLabel->setProperty("role", "coordinateTitle");
    cardLayout->addWidget(titleLabel);

    QHBoxLayout* valuesLayout = new QHBoxLayout;
    valuesLayout->setSpacing(14);
    for (const QString& axis : axes) {
        QLabel* value = new QLabel(QStringLiteral("%1  0.000 mm").arg(axis));
        value->setProperty("role", "coordinate");
        valuesLayout->addWidget(value);
    }
    cardLayout->addLayout(valuesLayout);
    return card;
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
    layout->addWidget(createCoordinateCard(QStringLiteral("电机绝对坐标"), {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}));
    layout->addWidget(createCoordinateCard(QStringLiteral("物料相对坐标"), {QStringLiteral("xw"), QStringLiteral("yw")}));
    layout->addWidget(createLaserMeasurementCard());

    runState_ = WidgetFactory::createStatusBadge(QStringLiteral("就绪"), QStringLiteral("ready"));
    runState_->setMinimumWidth(74);
    layout->addWidget(runState_);

    QPushButton* emergencyStop = WidgetFactory::createDangerButton(QStringLiteral("急停"));
    emergencyStop->setProperty("emergencyStop", true);
    emergencyStop->setMinimumSize(92, 52);
    emergencyStop->setToolTip(QStringLiteral("软件急停入口，不能替代物理急停与安全联锁"));
    connect(emergencyStop, &QPushButton::clicked, this, &TopStatusWidget::emergencyStopRequested);
    layout->addWidget(emergencyStop);
}

QFrame* TopStatusWidget::createLaserMeasurementCard()
{
    QFrame* card = new QFrame;
    card->setProperty("role", "coordinateCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 8, 14, 8);
    cardLayout->setSpacing(4);

    QLabel* title = new QLabel(QStringLiteral("激光测量"));
    title->setProperty("role", "coordinateTitle");
    cardLayout->addWidget(title);

    laserMeasurement_ = new QLabel(QStringLiteral("-- mm"));
    laserMeasurement_->setProperty("role", "coordinate");
    cardLayout->addWidget(laserMeasurement_);
    return card;
}

void TopStatusWidget::setRunState(const QString& state, const QString& styleClass)
{
    runState_->setText(state);
    runState_->setProperty("state", styleClass);
    runState_->style()->unpolish(runState_);
    runState_->style()->polish(runState_);
}

void TopStatusWidget::setLaserMeasurementMillimeters(double measurementMillimeters)
{
    laserMeasurement_->setText(
        std::isfinite(measurementMillimeters)
            ? QStringLiteral("%1 mm").arg(measurementMillimeters, 0, 'f', 3)
            : QStringLiteral("-- mm"));
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
    QFormLayout* deviceLayout = new QFormLayout(devices);
    motionState_ = WidgetFactory::createStatusBadge(QStringLiteral("离线"), QStringLiteral("offline"));
    deviceLayout->addRow(QStringLiteral("运动控制器"), motionState_);
    probeState_ = WidgetFactory::createStatusBadge(QStringLiteral("离线"), QStringLiteral("offline"));
    deviceLayout->addRow(QStringLiteral("激光探头"), probeState_);
    layout->addWidget(devices);

    QGroupBox* task = new QGroupBox(QStringLiteral("当前任务"));
    QVBoxLayout* taskLayout = new QVBoxLayout(task);
    taskState_ = new QLabel(QStringLiteral("等待操作"));
    taskState_->setProperty("role", "taskState");
    taskDetail_ = new QLabel(QStringLiteral("界面原型尚未连接业务服务"));
    taskDetail_->setWordWrap(true);
    taskDetail_->setProperty("role", "muted");
    taskLayout->addWidget(taskState_);
    taskLayout->addWidget(taskDetail_);
    layout->addWidget(task);

    QGroupBox* alert = new QGroupBox(QStringLiteral("报警与提示"));
    QVBoxLayout* alertLayout = new QVBoxLayout(alert);
    QLabel* noAlert = new QLabel(QStringLiteral("当前无活动报警"));
    noAlert->setWordWrap(true);
    alertLayout->addWidget(noAlert);
    layout->addWidget(alert);
    layout->addStretch();
}

void RightStatusWidget::setTaskState(const QString& state, const QString& detail)
{
    taskState_->setText(state);
    taskDetail_->setText(detail);
}

void RightStatusWidget::setMotionConnectionState(bool connected)
{
    motionState_->setText(connected ? QStringLiteral("在线") : QStringLiteral("离线"));
    motionState_->setProperty("state", connected ? QStringLiteral("ready") : QStringLiteral("offline"));
    motionState_->style()->unpolish(motionState_);
    motionState_->style()->polish(motionState_);
}

void RightStatusWidget::setProbeConnectionState(bool connected)
{
    probeState_->setText(connected ? QStringLiteral("在线") : QStringLiteral("离线"));
    probeState_->setProperty("state", connected ? QStringLiteral("ready") : QStringLiteral("offline"));
    probeState_->style()->unpolish(probeState_);
    probeState_->style()->polish(probeState_);
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
