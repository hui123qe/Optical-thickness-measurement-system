#include "measurement_database.h"

#include <cmath>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace {

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

} // namespace

MeasurementDatabase::MeasurementDatabase(QString filePath)
    : filePath_(std::move(filePath))
    , connectionName_(QStringLiteral("measurement-db-%1")
                          .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

MeasurementDatabase::~MeasurementDatabase()
{
    if (database_.isValid()) {
        database_.close();
        database_ = QSqlDatabase();
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool MeasurementDatabase::open(QString* errorMessage)
{
    if (database_.isOpen()) {
        return true;
    }

    const QFileInfo databaseFile(filePath_);
    QDir directory;
    if (!directory.mkpath(databaseFile.absolutePath())) {
        setError(errorMessage, QStringLiteral("无法创建数据库目录：%1").arg(databaseFile.absolutePath()));
        return false;
    }

    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(filePath_);
    if (!database_.open()) {
        setError(errorMessage, QStringLiteral("无法打开测量数据库：%1").arg(database_.lastError().text()));
        return false;
    }

    QSqlQuery pragma(database_);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));

    return executeSchemaStatement(
            QStringLiteral(
                "CREATE TABLE IF NOT EXISTS task_logs ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "run_id TEXT NOT NULL UNIQUE,"
                "task_type TEXT NOT NULL,"
                "finished_at_utc_ms INTEGER NOT NULL,"
                "result TEXT NOT NULL,"
                "detail TEXT NOT NULL)"),
            errorMessage)
        && executeSchemaStatement(
            QStringLiteral(
                "CREATE INDEX IF NOT EXISTS idx_task_logs_finished_at "
                "ON task_logs(finished_at_utc_ms DESC)"),
            errorMessage);
}

bool MeasurementDatabase::prepareExperiment(
    const QString& runId,
    QString* tableName,
    QString* errorMessage)
{
    if (!open(errorMessage)) {
        return false;
    }

    QString resolvedTableName;
    if (!resolveExperimentTableName(runId, resolvedTableName, errorMessage)) {
        return false;
    }
    const QString createTableSql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS %1 ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "point_index INTEGER NOT NULL UNIQUE,"
        "point_description TEXT NOT NULL,"
        "motor_x REAL NOT NULL,"
        "motor_y REAL NOT NULL,"
        "workpiece_x REAL NOT NULL,"
        "workpiece_y REAL NOT NULL,"
        "thickness_micrometers REAL NOT NULL,"
        "measured_at_utc_ms INTEGER NOT NULL)")
                                       .arg(resolvedTableName);
    if (!executeSchemaStatement(createTableSql, errorMessage)) {
        return false;
    }
    if (tableName != nullptr) {
        *tableName = resolvedTableName;
    }
    return true;
}

bool MeasurementDatabase::insertMeasurement(
    const QString& runId,
    int pointIndex,
    const QString& pointDescription,
    const QPointF& actualMotorPosition,
    const QPointF& workpiecePoint,
    double thicknessMicrometers,
    const QDateTime& measuredAt,
    QString* errorMessage)
{
    if (!open(errorMessage)) {
        return false;
    }
    if (runId.isEmpty() || pointIndex < 0 || pointDescription.trimmed().isEmpty()
        || !measuredAt.isValid()
        || !std::isfinite(actualMotorPosition.x()) || !std::isfinite(actualMotorPosition.y())
        || !std::isfinite(workpiecePoint.x()) || !std::isfinite(workpiecePoint.y())
        || !std::isfinite(thicknessMicrometers)) {
        setError(errorMessage, QStringLiteral("测量记录包含无效数据。"));
        return false;
    }

    QString tableName;
    if (!resolveExperimentTableName(runId, tableName, errorMessage)) {
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral(
        "INSERT INTO %1 ("
        "point_index, point_description, motor_x, motor_y, workpiece_x, workpiece_y, "
        "thickness_micrometers, measured_at_utc_ms) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)")
                      .arg(tableName));
    query.addBindValue(pointIndex + 1);
    query.addBindValue(pointDescription);
    query.addBindValue(actualMotorPosition.x());
    query.addBindValue(actualMotorPosition.y());
    query.addBindValue(workpiecePoint.x());
    query.addBindValue(workpiecePoint.y());
    query.addBindValue(thicknessMicrometers);
    query.addBindValue(measuredAt.toUTC().toMSecsSinceEpoch());
    if (!query.exec()) {
        setError(errorMessage, QStringLiteral("写入测量记录失败：%1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool MeasurementDatabase::insertTaskLog(
    const QString& runId,
    const QString& taskType,
    const QString& result,
    const QString& detail,
    const QDateTime& finishedAt,
    QString* errorMessage)
{
    if (!open(errorMessage)) {
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral(
        "INSERT INTO task_logs (run_id, task_type, finished_at_utc_ms, result, detail) "
        "VALUES (?, ?, ?, ?, ?)"));
    query.addBindValue(runId);
    query.addBindValue(taskType);
    query.addBindValue(finishedAt.toUTC().toMSecsSinceEpoch());
    query.addBindValue(result);
    query.addBindValue(detail);
    if (!query.exec()) {
        setError(errorMessage, QStringLiteral("写入任务日志失败：%1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

QList<TaskLogRecord> MeasurementDatabase::queryTaskLogs(
    const QDateTime& rangeStart,
    const QDateTime& rangeEnd,
    const QString& keyword,
    QString* errorMessage)
{
    QList<TaskLogRecord> records;
    if (!open(errorMessage)) {
        return records;
    }

    QSqlQuery query(database_);
    query.prepare(QStringLiteral(
        "SELECT id, run_id, task_type, finished_at_utc_ms, result, detail "
        "FROM task_logs "
        "WHERE finished_at_utc_ms >= ? AND finished_at_utc_ms <= ? "
        "AND (? = '' OR task_type LIKE ? OR result LIKE ? OR detail LIKE ?) "
        "ORDER BY finished_at_utc_ms DESC"));
    const QString normalizedKeyword = keyword.trimmed();
    const QString pattern = QStringLiteral("%%1%").arg(normalizedKeyword);
    query.addBindValue(rangeStart.toUTC().toMSecsSinceEpoch());
    query.addBindValue(rangeEnd.toUTC().toMSecsSinceEpoch());
    query.addBindValue(normalizedKeyword);
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    if (!query.exec()) {
        setError(errorMessage, QStringLiteral("查询任务日志失败：%1").arg(query.lastError().text()));
        return records;
    }

    while (query.next()) {
        records.append(TaskLogRecord{
            query.value(0).toLongLong(),
            query.value(1).toString(),
            query.value(2).toString(),
            QDateTime::fromMSecsSinceEpoch(query.value(3).toLongLong(), Qt::UTC).toLocalTime(),
            query.value(4).toString(),
            query.value(5).toString()});
    }
    return records;
}

QList<MeasurementRecord> MeasurementDatabase::queryMeasurements(
    const QString& runId,
    QString* errorMessage)
{
    QList<MeasurementRecord> records;
    if (!open(errorMessage)) {
        return records;
    }

    QString tableName;
    if (!resolveExperimentTableName(runId, tableName, errorMessage)) {
        return records;
    }

    QSqlQuery query(database_);
    const QString sql = QStringLiteral(
        "SELECT point_index, point_description, motor_x, motor_y, workpiece_x, workpiece_y, "
        "thickness_micrometers, measured_at_utc_ms "
        "FROM %1 ORDER BY point_index ASC")
                            .arg(tableName);
    if (!query.exec(sql)) {
        setError(
            errorMessage,
            QStringLiteral("查询实验测量记录失败：%1").arg(query.lastError().text()));
        return records;
    }

    while (query.next()) {
        records.append(MeasurementRecord{
            query.value(0).toInt(),
            query.value(1).toString(),
            QPointF(query.value(2).toDouble(), query.value(3).toDouble()),
            QPointF(query.value(4).toDouble(), query.value(5).toDouble()),
            query.value(6).toDouble(),
            QDateTime::fromMSecsSinceEpoch(query.value(7).toLongLong(), Qt::UTC)
                .toLocalTime()});
    }
    return records;
}

QString MeasurementDatabase::defaultFilePath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("data/measurement-history.sqlite"));
}

bool MeasurementDatabase::executeSchemaStatement(const QString& sql, QString* errorMessage)
{
    QSqlQuery query(database_);
    if (!query.exec(sql)) {
        setError(errorMessage, QStringLiteral("初始化测量数据库失败：%1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool MeasurementDatabase::resolveExperimentTableName(
    const QString& runId,
    QString& tableName,
    QString* errorMessage)
{
    const QUuid uuid(runId);
    if (uuid.isNull()) {
        setError(errorMessage, QStringLiteral("实验运行 ID 无效，无法确定数据表。"));
        return false;
    }

    QString compactId = uuid.toString(QUuid::WithoutBraces);
    compactId.remove(QLatin1Char('-'));
    tableName = QStringLiteral("experiment_%1").arg(compactId);
    return true;
}
