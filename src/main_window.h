#pragma once

#include "workflow/workflow_policy.h"

#include <array>
#include <optional>

#include <QMainWindow>

class RightStatusWidget;
class LogPage;
class MainPage;
class MeasurementTaskController;
class NavigationWidget;
class QStackedWidget;
class QString;
class TaskExecutor;
class TopStatusWidget;

namespace otms::device {
class DeviceManager;
}

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void connectDeviceSignals(
        otms::device::DeviceManager& deviceManager,
        MainPage* mainPage);
    void connectWorkflowSignals(MainPage* mainPage, LogPage* logPage);
    void connectWindowActions(NavigationWidget* navigation, QStackedWidget* pageStack);
    void connectMainPageActions(
        MainPage* mainPage,
        otms::device::DeviceManager& deviceManager);
    void setTaskState(const QString& state, const QString& detail, const QString& styleClass);
    void exportCurrentPosition();
    void showPrototypeNotice(const QString& action);
    void showMotionConnectionDialog();
    void showLaserConnectionDialog();

    otms::workflow::WorkflowPolicy workflowPolicy_;
    TopStatusWidget* topStatus_{};
    RightStatusWidget* rightStatus_{};
    TaskExecutor* taskExecutor_{};
    MeasurementTaskController* measurementTaskController_{};
    std::optional<std::array<double, 3>> latestMotorPositionMillimeters_;
    std::optional<std::array<double, 2>> latestWorkpiecePositionMillimeters_;
    std::optional<double> latestThicknessMillimeters_;
};
