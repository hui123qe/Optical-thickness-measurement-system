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

    void setRunState(const QString& state, const QString& styleClass);

public slots:
    void setLaserMeasurementMillimeters(double measurementMillimeters);

signals:
    void emergencyStopRequested();

private:
    QFrame* createLaserMeasurementCard();

    QLabel* laserMeasurement_{};
    QLabel* runState_{};
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
    void setTaskState(const QString& state, const QString& detail);
    void setMotionConnectionState(bool connected);
    void setProbeConnectionState(bool connected);

private:
    QLabel* motionState_{};
    QLabel* probeState_{};
    QLabel* taskState_{};
    QLabel* taskDetail_{};
};

class BottomStatusBar final : public QStatusBar
{
public:
    explicit BottomStatusBar(QWidget* parent = nullptr);
};
