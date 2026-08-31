#pragma once

#include "../data/measurement_database.h"

#include <QWidget>

class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTimer;

class LogPage final : public QWidget
{
    Q_OBJECT

public:
    explicit LogPage(QWidget* parent = nullptr);

public slots:
    void refresh();
    void exportSelectedRecord();

private:
    MeasurementDatabase database_;
    QDateTimeEdit* startTime_{};
    QDateTimeEdit* endTime_{};
    QLineEdit* keyword_{};
    QLabel* queryStatus_{};
    QTableWidget* logTable_{};
    QPushButton* exportButton_{};
    QTimer* filterTimer_{};
};
