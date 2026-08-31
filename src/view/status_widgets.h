#pragma once

#include "../workflow/machine_state.h"
#include "../workflow/workflow_policy.h"

#include <QStatusBar>
#include <QWidget>

class QLabel;
class QListWidget;
class QFrame;
class QPushButton;
class QString;

class TopStatusWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit TopStatusWidget(QWidget* parent = nullptr);

public slots:
    void setMotionConnectionState(bool connected);
    void setMotorPositionMillimeters(
        double xMillimeters,
        double yMillimeters,
        double zMillimeters);
    void setWorkpiecePositionMillimeters(
        double xMillimeters,
        double yMillimeters);
    void setLaserConnectionState(bool connected);
    void setLaserMeasurementState(bool measuring);
    void setLaserMeasurementMillimeters(double measurementMillimeters);
    void setOperationProfile(otms::workflow::OperationProfile profile);

signals:
    void operationProfileChangeRequested(otms::workflow::OperationProfile profile);
    void emergencyStopRequested();
    void resetRequested();
    void doorLockCommandRequested(bool locked);

private:
    QFrame* createMotorPositionCard();
    QFrame* createWorkpiecePositionCard();
    QFrame* createLaserMeasurementCard();

    QLabel* motorXPosition_{};
    QLabel* motorYPosition_{};
    QLabel* motorZPosition_{};
    QLabel* workpieceXPosition_{};
    QLabel* workpieceYPosition_{};
    QLabel* laserMeasurement_{};
    QPushButton* debugBypassButton_{};
    otms::workflow::OperationProfile currentOperationProfile_{
        otms::workflow::OperationProfile::Production};
    bool motionConnected_{};
    bool laserConnected_{};
};

class NavigationWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit NavigationWidget(QWidget* parent = nullptr);
    void selectPage(int index);

signals:
    void pageSelected(int index);

private:
    QListWidget* navigation_{};
};

class RightStatusWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit RightStatusWidget(QWidget* parent = nullptr);
    void setMotionConnectionState(bool connected);
    void setProbeConnectionState(bool connected);
    void setDoorLockState(bool available, bool locked);
    void setLightCurtainState(bool available, bool clear);
    void setMachineState(otms::workflow::MachineState state);
    void setRunMode(otms::workflow::RunMode mode);

signals:
    void motionConnectionRequested();
    void laserConnectionRequested();
    void runModeChangeRequested(otms::workflow::RunMode mode);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QWidget* motionConnectionArea_{};
    QWidget* laserConnectionArea_{};
    QLabel* motionState_{};
    QLabel* probeState_{};
    QLabel* doorLockState_{};
    QLabel* lightCurtainState_{};
    QLabel* machineState_{};
    QLabel* runMode_{};
    otms::workflow::RunMode currentRunMode_{otms::workflow::RunMode::Manual};
};

class BottomStatusBar final : public QStatusBar
{
public:
    explicit BottomStatusBar(QWidget* parent = nullptr);
};
