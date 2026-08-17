#pragma once

#include <QMainWindow>

class RightStatusWidget;
class MeasurementTaskController;
class QString;
class TaskExecutor;
class TopStatusWidget;

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void setTaskState(const QString& state, const QString& detail, const QString& styleClass);
    void showPrototypeNotice(const QString& action);
    void showMotionConnectionDialog();
    void showLaserConnectionDialog();

    TopStatusWidget* topStatus_{};
    RightStatusWidget* rightStatus_{};
    TaskExecutor* taskExecutor_{};
    MeasurementTaskController* measurementTaskController_{};
};
