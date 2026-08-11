#pragma once

#include <QStatusBar>
#include <QWidget>

class QLabel;
class QListWidget;
class QFrame;
class QString;

class TopStatusWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit TopStatusWidget(QWidget* parent = nullptr);

public slots:
    void setMotionConnectionState(bool connected);
    void setMotorPositionMillimeters(double xMillimeters, double yMillimeters);
    void setLaserConnectionState(bool connected);
    void setLaserMeasurementMillimeters(double measurementMillimeters);

signals:
    void emergencyStopRequested();

private:
    QFrame* createMotorPositionCard();
    QFrame* createWorkpiecePositionCard();
    QFrame* createLaserMeasurementCard();

    QLabel* motorXPosition_{};
    QLabel* motorYPosition_{};
    QLabel* laserMeasurement_{};
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

private:
    QLabel* motionState_{};
    QLabel* probeState_{};
};

class BottomStatusBar final : public QStatusBar
{
public:
    explicit BottomStatusBar(QWidget* parent = nullptr);
};
