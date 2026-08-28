#pragma once

#include "../workflow/task_executor.h"
#include "../workflow/task_point_queue.h"

#include <QList>
#include <QPointF>
#include <QStringList>
#include <QTransform>
#include <QWidget>

#include <optional>

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QString;
class QTabWidget;
class ManualControlWidget;

enum class MotorAxis
{
    X,
    Y,
    Z
};

class MainPage final : public QWidget
{
    Q_OBJECT

public:
    explicit MainPage(QWidget* parent = nullptr);
    [[nodiscard]] std::optional<QPointF> workpiecePositionForMotor(
        double motorX,
        double motorY) const;

public slots:
    void setLaserConnectionState(bool connected);
    void setLaserMeasurementState(bool measuring);
    void setAxisEnableState(MotorAxis axis, bool available, bool enabled);

signals:
    void stageAxisAbsoluteMoveRequested(MotorAxis axis, double target);
    void stageAxisRelativeMoveRequested(MotorAxis axis, double distance);
    void stageAxisHomeRequested(MotorAxis axis);
    void stageAxisEnableRequested(MotorAxis axis, bool enabled);
    void stageHomeRequested();
    void stageAbsoluteMoveRequested(double motorX, double motorY, double motorZ);
    void workpiecePointMoveRequested(double workpieceX, double workpieceY, double motorX, double motorY);
    void currentPositionExportRequested();
    void laserMeasurementStartRequested();
    void laserMeasurementStopRequested();
    void taskStateChangeRequested(const QString& state, const QString& detail, const QString& styleClass);
    void automaticTaskPrepared(
        const QString& taskType,
        const QList<TaskExecutionPoint>& points);
    void taskTerminationRequested();

private slots:
    void applyCoordinateTransform(int index);
    void forwardWorkpiecePointMove(double workpieceX, double workpieceY);
    void prepareAutomaticMotorPointQueue(
        const QString& taskType,
        const QList<WorkpieceTaskPoint>& points);

private:
    QGroupBox* createCoordinateTransformGroup();
    QHBoxLayout* createLaserMeasurementControls();
    QTabWidget* createOperationTabs();
    void updateAutomaticMotorPointQueue();
    void updateLaserMeasurementControls();

    QComboBox* transformSelector_{};
    QLabel* transformParameters_{};
    QList<QTransform> coordinateTransforms_;
    QStringList coordinateTransformDescriptions_;
    QList<WorkpieceTaskPoint> automaticWorkpiecePointQueue_;
    QList<TaskExecutionPoint> automaticTaskPointQueue_;
    QTransform workpieceToMotorTransform_;
    QTransform motorToWorkpieceTransform_;
    bool coordinateTransformAvailable_{};
    QPushButton* startLaserMeasurement_{};
    QPushButton* stopLaserMeasurement_{};
    QLabel* laserMeasurementState_{};
    ManualControlWidget* manualControl_{};
    bool laserConnected_{};
    bool laserMeasuring_{};
};

class ManualControlWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ManualControlWidget(QWidget* parent = nullptr);

public slots:
    void setAxisEnableState(MotorAxis axis, bool available, bool enabled);

signals:
    void stageAxisAbsoluteMoveRequested(MotorAxis axis, double target);
    void stageAxisRelativeMoveRequested(MotorAxis axis, double distance);
    void stageAxisHomeRequested(MotorAxis axis);
    void stageAxisEnableRequested(MotorAxis axis, bool enabled);
    void stageHomeRequested();
    void stageAbsoluteMoveRequested(double motorX, double motorY, double motorZ);
    void workpiecePointMoveRequested(double workpieceX, double workpieceY);
    void currentPositionExportRequested();

private slots:
    void requestXAxisMove();
    void requestYAxisMove();
    void requestZAxisMove();
    void requestXAxisRelativeMove();
    void requestYAxisRelativeMove();
    void requestZAxisRelativeMove();
    void requestStageMove();
    void requestWorkpiecePointMove();

private:
    QGroupBox* createAbsoluteMoveGroup();
    QGroupBox* createWorkpieceMoveGroup();
    QGroupBox* createPositionExportGroup();

    QDoubleSpinBox* motorXInput_{};
    QDoubleSpinBox* motorYInput_{};
    QDoubleSpinBox* motorZInput_{};
    QDoubleSpinBox* motorXRelativeInput_{};
    QDoubleSpinBox* motorYRelativeInput_{};
    QDoubleSpinBox* motorZRelativeInput_{};
    QDoubleSpinBox* workpieceXInput_{};
    QDoubleSpinBox* workpieceYInput_{};
    QLabel* xAxisEnableState_{};
    QLabel* yAxisEnableState_{};
    QLabel* zAxisEnableState_{};
};
