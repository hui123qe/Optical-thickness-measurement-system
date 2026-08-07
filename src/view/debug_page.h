#pragma once

#include <QList>
#include <QWidget>

#include "../device/laser_probe.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTextEdit;

class LaserDebugPage final : public QWidget
{
    Q_OBJECT

public:
    explicit LaserDebugPage(QWidget* parent = nullptr);

private:
    bool applyProbeConfig();
    otms::device::LaserOutput selectedOutput() const;
    void appendStatus(const QString& operation, const otms::device::LaserStatus& status);
    void showMeasurement(const otms::device::LaserMeasurement& measurement);
    void updateConnectionState(bool connected, const QString& detail);
    void updateControls();
    bool confirmHardwareAction(const QString& title, const QString& detail);

    QSpinBox* programInput_{};
    QComboBox* outputInput_{};
    QDoubleSpinBox* scaleInput_{};
    QSpinBox* measurementTimeoutInput_{};
    QSpinBox* pollingIntervalInput_{};

    QLabel* connectionState_{};
    QLabel* rawValue_{};
    QLabel* convertedValue_{};
    QLabel* quality_{};
    QLabel* judgment_{};
    QLabel* triggerCount_{};
    QTextEdit* operationLog_{};

    QPushButton* connectButton_{};
    QPushButton* disconnectButton_{};
    QList<QPushButton*> connectedControls_;
};
