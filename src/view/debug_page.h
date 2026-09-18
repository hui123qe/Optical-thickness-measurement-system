#pragma once

#include <QList>
#include <QWidget>

#include "../device/laser_probe.h"

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTextEdit;

class LaserDebugPage final : public QWidget
{
    Q_OBJECT

public:
    explicit LaserDebugPage(QWidget* parent = nullptr);

private slots:
    void applyProbeConfigFromControl();
    void saveProbeConfig();
    void connectLaser();
    void disconnectLaser();
    void selectLaserProgram(int index);
    void enableLaser();
    void disableLaser();
    void startLaserMeasurement();
    void stopLaserMeasurement();
    void setLaserZero();
    void clearLaserZero();
    void resetLaserOutput();
    void readLatestLaserMeasurement();
    void measureLaserOnce();

private:
    bool applyProbeConfig();
    otms::device::LaserOutput selectedOutput() const;
    void appendStatus(const QString& operation, const otms::device::LaserStatus& status);
    void showMeasurement(const otms::device::LaserMeasurement& measurement);
    void updateConnectionState(bool connected, const QString& detail);
    void updateControls();
    bool confirmHardwareAction(const QString& title, const QString& detail);

    QComboBox* programInput_{};
    QComboBox* outputInput_{};
    QSpinBox* measurementTimeoutInput_{};
    QSpinBox* pollingIntervalInput_{};

    QLabel* connectionState_{};
    QLabel* rawValue_{};
    QLabel* displayUnit_{};
    QLabel* measuredValue_{};
    QLabel* quality_{};
    QLabel* judgment_{};
    QLabel* triggerCount_{};
    QTextEdit* operationLog_{};

    QPushButton* connectButton_{};
    QPushButton* disconnectButton_{};
    QList<QPushButton*> connectedControls_;
};
