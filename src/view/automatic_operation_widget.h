#pragma once

#include "../config/task_configuration.h"
#include "../config/task_configuration_store.h"
#include "../workflow/task_point_queue.h"

#include <array>

#include <QList>
#include <QPointF>
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QString;
class QTableWidget;

class AutomaticOperationWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit AutomaticOperationWidget(QWidget* parent = nullptr);

signals:
    void taskStateChangeRequested(const QString& state, const QString& detail, const QString& styleClass);
    void workpiecePointQueuePrepared(
        const QString& taskType,
        const QList<WorkpieceTaskPoint>& points);
    void taskTerminationRequested();

private slots:
    void setCircleRingOverride();
    void removeSelectedCircleRingOverride();
    void loadSelectedCircleRingOverride();
    void updateCircleOverrideRingRange(int ringCount);

private:
    [[nodiscard]] TaskConfiguration configurationFromEditor() const;
    void applyConfiguration(const TaskConfiguration& configuration);
    [[nodiscard]] int findCircleRingOverrideRow(int ringIndex) const;
    void upsertCircleRingOverride(int ringIndex, int pointCount);
    void setConfigurationLocked(bool locked);
    void loadConfiguration();
    void saveConfiguration();
    void resetConfiguration();
    void confirmConfiguration();
    void startTask();

    TaskConfigurationStore configurationStore_;
    TaskConfiguration confirmedConfiguration_;
    bool configurationLocked_{};

    QComboBox* taskMode_{};
    QStackedWidget* modeConfiguration_{};
    std::array<QDoubleSpinBox*, 4> rectangleCornerXInputs_{};
    std::array<QDoubleSpinBox*, 4> rectangleCornerYInputs_{};
    QSpinBox* rectangleRowCount_{};
    QSpinBox* rectangleColumnCount_{};
    QDoubleSpinBox* circleCenterX_{};
    QDoubleSpinBox* circleCenterY_{};
    QDoubleSpinBox* circleMaximumRadius_{};
    QSpinBox* circleRingCount_{};
    QSpinBox* circleDefaultPointCount_{};
    QSpinBox* circleOverrideRingIndex_{};
    QSpinBox* circleOverridePointCount_{};
    QTableWidget* circleRingOverrides_{};
    QPushButton* setCircleOverrideButton_{};
    QPushButton* removeCircleOverrideButton_{};
    QLineEdit* csvPath_{};
    QPushButton* browseButton_{};
    QPushButton* loadButton_{};
    QPushButton* saveButton_{};
    QPushButton* resetButton_{};
    QPushButton* confirmButton_{};
    QPushButton* startButton_{};
    QPushButton* terminateButton_{};
};
