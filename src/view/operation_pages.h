#pragma once

#include "../workflow/task_executor.h"
#include "../workflow/task_point_queue.h"

#include <QList>
#include <QPointF>
#include <QStringList>
#include <QTransform>
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QString;
class QTabWidget;

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

public slots:
    void setLaserConnectionState(bool connected);
    void setLaserMeasurementState(bool measuring);

signals:
    void stageAxisAbsoluteMoveRequested(MotorAxis axis, double target);
    void stageAxisRelativeMoveRequested(MotorAxis axis, double distance);
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
    QPushButton* startLaserMeasurement_{};
    QPushButton* stopLaserMeasurement_{};
    QLabel* laserMeasurementState_{};
    bool laserConnected_{};
    bool laserMeasuring_{};
};

class ManualControlWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ManualControlWidget(QWidget* parent = nullptr);

signals:
    void stageAxisAbsoluteMoveRequested(MotorAxis axis, double target);
    void stageAxisRelativeMoveRequested(MotorAxis axis, double distance);
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
};
