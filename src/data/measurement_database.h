#pragma once

#include <QDateTime>
#include <QList>
#include <QPointF>
#include <QSqlDatabase>
#include <QString>

struct TaskLogRecord
{
    qint64 id{};
    QString runId;
    QString taskType;
    QDateTime finishedAt;
    QString result;
    QString detail;
};

struct MeasurementRecord
{
    int pointIndex{};
    QString pointDescription;
    QPointF motorPosition;
    QPointF workpiecePosition;
    double thicknessMicrometers{};
    QDateTime measuredAt;
};

class MeasurementDatabase final
{
public:
    explicit MeasurementDatabase(QString filePath = defaultFilePath());
    ~MeasurementDatabase();

    MeasurementDatabase(const MeasurementDatabase&) = delete;
    MeasurementDatabase& operator=(const MeasurementDatabase&) = delete;

    bool open(QString* errorMessage = nullptr);
    bool prepareExperiment(
        const QString& runId,
        QString* tableName = nullptr,
        QString* errorMessage = nullptr);
    bool insertMeasurement(
        const QString& runId,
        int pointIndex,
        const QString& pointDescription,
        const QPointF& actualMotorPosition,
        const QPointF& workpiecePoint,
        double thicknessMicrometers,
        const QDateTime& measuredAt,
        QString* errorMessage = nullptr);
    bool insertTaskLog(
        const QString& runId,
        const QString& taskType,
        const QString& result,
        const QString& detail,
        const QDateTime& finishedAt,
        QString* errorMessage = nullptr);
    QList<TaskLogRecord> queryTaskLogs(
        const QDateTime& rangeStart,
        const QDateTime& rangeEnd,
        const QString& keyword,
        QString* errorMessage = nullptr);
    QList<MeasurementRecord> queryMeasurements(
        const QString& runId,
        QString* errorMessage = nullptr);

    [[nodiscard]] static QString defaultFilePath();

private:
    bool executeSchemaStatement(const QString& sql, QString* errorMessage);
    static bool resolveExperimentTableName(
        const QString& runId,
        QString& tableName,
        QString* errorMessage);

    QString filePath_;
    QString connectionName_;
    QSqlDatabase database_;
};
