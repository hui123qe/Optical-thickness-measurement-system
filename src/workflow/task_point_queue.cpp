#include "task_point_queue.h"

#include <cmath>
#include <numbers>

#include <QFile>
#include <QStringConverter>
#include <QStringList>
#include <QTextStream>

namespace {

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

QList<WorkpieceTaskPoint> createRectanglePointQueue(const RectangleTaskConfiguration& rectangle)
{
    const RectangleGridCorners corners = *rectangle.normalizedGridCorners();

    QList<WorkpieceTaskPoint> points;
    points.reserve(rectangle.rowCount * rectangle.columnCount);
    for (int rowIndex = 0; rowIndex < rectangle.rowCount; ++rowIndex) {
        const double rowFactor = rectangle.rowCount == 1
            ? 0.5
            : static_cast<double>(rowIndex) / static_cast<double>(rectangle.rowCount - 1);
        const QPointF rowStart = corners.origin
            + rowFactor * (corners.rowEnd - corners.origin);
        const QPointF rowEnd = corners.columnEnd
            + rowFactor * (corners.opposite - corners.columnEnd);

        for (int queueColumnIndex = 0;
             queueColumnIndex < rectangle.columnCount;
             ++queueColumnIndex) {
            const int columnIndex = rowIndex % 2 == 0
                ? queueColumnIndex
                : rectangle.columnCount - 1 - queueColumnIndex;
            const double columnFactor = rectangle.columnCount == 1
                ? 0.5
                : static_cast<double>(columnIndex)
                    / static_cast<double>(rectangle.columnCount - 1);
            points.append(WorkpieceTaskPoint{
                rowStart + columnFactor * (rowEnd - rowStart),
                QStringLiteral("第%1行第%2列").arg(rowIndex + 1).arg(columnIndex + 1)});
        }
    }
    return points;
}

QList<WorkpieceTaskPoint> createCirclePointQueue(const CircleTaskConfiguration& circle)
{
    int totalPointCount = 1;
    for (int ringIndex = 1; ringIndex <= circle.ringCount; ++ringIndex) {
        totalPointCount += circle.pointCountForRing(ringIndex);
    }

    QList<WorkpieceTaskPoint> points;
    points.reserve(totalPointCount);
    points.append(WorkpieceTaskPoint{
        QPointF(circle.centerX, circle.centerY),
        QStringLiteral("圆心")});
    for (int ringIndex = 1; ringIndex <= circle.ringCount; ++ringIndex) {
        const double radius = circle.maximumRadius
            * static_cast<double>(ringIndex)
            / static_cast<double>(circle.ringCount);
        const int pointCount = circle.pointCountForRing(ringIndex);
        const double angleStep = 2.0 * std::numbers::pi_v<double>
            / static_cast<double>(pointCount);
        for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
            const double angle = pointIndex * angleStep;
            points.append(WorkpieceTaskPoint{
                QPointF(
                    circle.centerX + radius * std::cos(angle),
                    circle.centerY + radius * std::sin(angle)),
                QStringLiteral("第%1圆环第%2点").arg(ringIndex).arg(pointIndex + 1)});
        }
    }
    return points;
}

std::optional<QList<WorkpieceTaskPoint>> loadCsvPointQueue(
    const CsvTaskConfiguration& csv,
    QString* errorMessage)
{
    QFile file(csv.resolvedFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage, QStringLiteral("无法读取 CSV 文件：%1").arg(file.errorString()));
        return std::nullopt;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QList<WorkpieceTaskPoint> points;
    qsizetype lineNumber = 0;
    bool firstContentLine = true;
    while (!stream.atEnd()) {
        ++lineNumber;
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const QStringList fields = line.split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (firstContentLine && fields.size() == 2
            && fields.at(0).trimmed().compare(QStringLiteral("xw"), Qt::CaseInsensitive) == 0
            && fields.at(1).trimmed().compare(QStringLiteral("yw"), Qt::CaseInsensitive) == 0) {
            firstContentLine = false;
            continue;
        }
        firstContentLine = false;

        if (fields.size() != 2) {
            setError(errorMessage, QStringLiteral("CSV 第 %1 行必须恰好包含 xw,yw 两列。").arg(lineNumber));
            return std::nullopt;
        }

        bool xValid = false;
        bool yValid = false;
        const double x = fields.at(0).trimmed().toDouble(&xValid);
        const double y = fields.at(1).trimmed().toDouble(&yValid);
        if (!xValid || !yValid || !std::isfinite(x) || !std::isfinite(y)) {
            setError(errorMessage, QStringLiteral("CSV 第 %1 行包含无效坐标。").arg(lineNumber));
            return std::nullopt;
        }
        points.append(WorkpieceTaskPoint{
            QPointF(x, y),
            QStringLiteral("第%1点").arg(points.size() + 1)});
    }

    if (points.isEmpty()) {
        setError(errorMessage, QStringLiteral("CSV 文件中没有有效坐标点。"));
        return std::nullopt;
    }
    return points;
}

} // namespace

std::optional<QList<WorkpieceTaskPoint>> createWorkpiecePointQueue(
    const TaskConfiguration& configuration,
    QString* errorMessage)
{
    const QString validationError = configuration.validationError();
    if (!validationError.isEmpty()) {
        setError(errorMessage, validationError);
        return std::nullopt;
    }

    switch (configuration.mode) {
    case TaskMode::Rectangle:
        return createRectanglePointQueue(configuration.rectangle);
    case TaskMode::Circle:
        return createCirclePointQueue(configuration.circle);
    case TaskMode::Csv:
        return loadCsvPointQueue(configuration.csv, errorMessage);
    }

    setError(errorMessage, QStringLiteral("任务模式无效。"));
    return std::nullopt;
}
