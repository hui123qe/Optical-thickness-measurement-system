#include "task_configuration.h"

#include <algorithm>
#include <cmath>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QStringList>

namespace {

constexpr double rectangleTolerance = 1.0e-6;
constexpr long long maximumTaskPointCount = 10000;

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

double squaredLength(const QPointF& vector)
{
    return vector.x() * vector.x() + vector.y() * vector.y();
}

double dotProduct(const QPointF& left, const QPointF& right)
{
    return left.x() * right.x() + left.y() * right.y();
}

bool nearlyEqual(double left, double right, double scale)
{
    return std::abs(left - right) <= rectangleTolerance * std::max(scale, 1.0);
}

} // namespace

std::optional<RectangleGridCorners> RectangleTaskConfiguration::normalizedGridCorners(
    QString* errorMessage) const
{
    std::array<QPointF, 4> orderedCorners = corners;
    for (const QPointF& corner : orderedCorners) {
        if (!std::isfinite(corner.x()) || !std::isfinite(corner.y())) {
            setError(errorMessage, QStringLiteral("矩形角点坐标必须是有限数值。"));
            return std::nullopt;
        }
    }

    QPointF center;
    for (const QPointF& corner : orderedCorners) {
        center += corner;
    }
    center /= static_cast<double>(orderedCorners.size());
    std::sort(
        orderedCorners.begin(),
        orderedCorners.end(),
        [center](const QPointF& left, const QPointF& right) {
            return std::atan2(left.y() - center.y(), left.x() - center.x())
                < std::atan2(right.y() - center.y(), right.x() - center.x());
        });

    std::array<QPointF, 4> edges;
    std::array<double, 4> squaredEdgeLengths{};
    double lengthScale = 0.0;
    for (std::size_t index = 0; index < orderedCorners.size(); ++index) {
        const std::size_t nextIndex = (index + 1) % orderedCorners.size();
        edges[index] = orderedCorners[nextIndex] - orderedCorners[index];
        squaredEdgeLengths[index] = squaredLength(edges[index]);
        lengthScale = std::max(lengthScale, squaredEdgeLengths[index]);
    }

    for (double squaredEdgeLength : squaredEdgeLengths) {
        if (squaredEdgeLength <= rectangleTolerance * rectangleTolerance) {
            setError(errorMessage, QStringLiteral("矩形四个角点不能重复或重合。"));
            return std::nullopt;
        }
    }

    for (std::size_t index = 0; index < edges.size(); ++index) {
        const std::size_t nextIndex = (index + 1) % edges.size();
        const double perpendicularScale =
            std::sqrt(squaredEdgeLengths[index] * squaredEdgeLengths[nextIndex]);
        if (std::abs(dotProduct(edges[index], edges[nextIndex]))
            > rectangleTolerance * perpendicularScale) {
            setError(errorMessage, QStringLiteral("四个角点必须构成矩形，角点输入顺序不限。"));
            return std::nullopt;
        }
    }
    if (!nearlyEqual(squaredEdgeLengths[0], squaredEdgeLengths[2], lengthScale)
        || !nearlyEqual(squaredEdgeLengths[1], squaredEdgeLengths[3], lengthScale)) {
        setError(errorMessage, QStringLiteral("四个角点必须构成矩形，且对边长度应一致。"));
        return std::nullopt;
    }

    const QPointF firstEdge = orderedCorners[1] - orderedCorners[0];
    const QPointF secondEdge = orderedCorners[3] - orderedCorners[0];
    const double firstLength = squaredEdgeLengths[0];
    const double secondLength = squaredEdgeLengths[3];
    bool firstEdgeIsColumnDirection = firstLength > secondLength;
    if (nearlyEqual(firstLength, secondLength, lengthScale)) {
        const double firstXAxisAlignment = std::abs(firstEdge.x()) / std::sqrt(firstLength);
        const double secondXAxisAlignment = std::abs(secondEdge.x()) / std::sqrt(secondLength);
        firstEdgeIsColumnDirection = firstXAxisAlignment >= secondXAxisAlignment;
    }

    return firstEdgeIsColumnDirection
        ? RectangleGridCorners{
              orderedCorners[0], orderedCorners[1], orderedCorners[3], orderedCorners[2]}
        : RectangleGridCorners{
              orderedCorners[0], orderedCorners[3], orderedCorners[1], orderedCorners[2]};
}

QString CsvTaskConfiguration::resolvedFilePath() const
{
    const QFileInfo fileInfo(filePath);
    if (fileInfo.isAbsolute()) {
        return fileInfo.absoluteFilePath();
    }
    return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(filePath);
}

QString TaskConfiguration::validationError() const
{
    switch (mode) {
    case TaskMode::Rectangle:
        {
            QString geometryError;
            if (!rectangle.normalizedGridCorners(&geometryError).has_value()) {
                return geometryError;
            }
        }
        if (rectangle.rowCount < 1 || rectangle.columnCount < 1) {
            return QStringLiteral("矩形阵列的行数和列数必须至少为 1。");
        }
        if (static_cast<long long>(rectangle.rowCount) * rectangle.columnCount
            > maximumTaskPointCount) {
            return QStringLiteral("矩形阵列的总点数不能超过 %1。")
                .arg(maximumTaskPointCount);
        }
        break;
    case TaskMode::Circle:
        if (!std::isfinite(circle.centerX) || !std::isfinite(circle.centerY)) {
            return QStringLiteral("圆形阵列的圆心坐标必须是有限数值。");
        }
        if (!std::isfinite(circle.maximumRadius) || circle.maximumRadius <= 0.0) {
            return QStringLiteral("圆形阵列的最大半径必须大于 0。");
        }
        if (circle.ringCount < 1) {
            return QStringLiteral("圆形阵列的圆环数量必须至少为 1。");
        }
        if (circle.defaultPointCount < 3) {
            return QStringLiteral("圆形阵列的默认每圈点数必须至少为 3。");
        }
        {
            QSet<int> overriddenRings;
            for (const CircleRingPointOverride& pointOverride : circle.ringPointOverrides) {
                if (pointOverride.ringIndex < 1 || pointOverride.ringIndex > circle.ringCount) {
                    return QStringLiteral("圆环覆盖序号 %1 超出 1～%2 的范围。")
                        .arg(pointOverride.ringIndex)
                        .arg(circle.ringCount);
                }
                if (pointOverride.pointCount < 3) {
                    return QStringLiteral("第 %1 圈的覆盖点数必须至少为 3。")
                        .arg(pointOverride.ringIndex);
                }
                if (overriddenRings.contains(pointOverride.ringIndex)) {
                    return QStringLiteral("第 %1 圈存在重复的点数覆盖配置。")
                        .arg(pointOverride.ringIndex);
                }
                overriddenRings.insert(pointOverride.ringIndex);
            }
        }
        {
            long long totalPointCount = 1;
            for (int ringIndex = 1; ringIndex <= circle.ringCount; ++ringIndex) {
                totalPointCount += circle.pointCountForRing(ringIndex);
            }
            if (totalPointCount > maximumTaskPointCount) {
                return QStringLiteral("圆形阵列包含圆心在内的总点数不能超过 %1。")
                    .arg(maximumTaskPointCount);
            }
        }
        break;
    case TaskMode::Csv:
        if (csv.filePath.trimmed().isEmpty()) {
            return QStringLiteral("请选择坐标 CSV 文件。");
        }
        if (!QFileInfo::exists(csv.resolvedFilePath())) {
            return QStringLiteral("所选 CSV 文件不存在，请重新选择。");
        }
        break;
    }
    return {};
}

QString TaskConfiguration::modeName() const
{
    switch (mode) {
    case TaskMode::Rectangle:
        return QStringLiteral("矩形阵列");
    case TaskMode::Circle:
        return QStringLiteral("圆形阵列");
    case TaskMode::Csv:
        return QStringLiteral("加载 CSV 文件");
    }
    return QStringLiteral("未知任务");
}

QString TaskConfiguration::summary() const
{
    switch (mode) {
    case TaskMode::Rectangle:
        return QStringLiteral(
                   "模式：矩形阵列\n"
                   "角点 1：(%1, %2) mm\n"
                   "角点 2：(%3, %4) mm\n"
                   "角点 3：(%5, %6) mm\n"
                   "角点 4：(%7, %8) mm\n"
                   "网格：%9 行 × %10 列，共 %11 点\n"
                   "队列：蛇形；列沿较长边，正方形时沿更接近物料 X 轴的边")
            .arg(rectangle.corners[0].x(), 0, 'f', 3)
            .arg(rectangle.corners[0].y(), 0, 'f', 3)
            .arg(rectangle.corners[1].x(), 0, 'f', 3)
            .arg(rectangle.corners[1].y(), 0, 'f', 3)
            .arg(rectangle.corners[2].x(), 0, 'f', 3)
            .arg(rectangle.corners[2].y(), 0, 'f', 3)
            .arg(rectangle.corners[3].x(), 0, 'f', 3)
            .arg(rectangle.corners[3].y(), 0, 'f', 3)
            .arg(rectangle.rowCount)
            .arg(rectangle.columnCount)
            .arg(rectangle.rowCount * rectangle.columnCount);
    case TaskMode::Circle:
        {
            QStringList overrideDescriptions;
            for (const CircleRingPointOverride& pointOverride : circle.ringPointOverrides) {
                overrideDescriptions.append(
                    QStringLiteral("第 %1 圈=%2 点")
                        .arg(pointOverride.ringIndex)
                        .arg(pointOverride.pointCount));
            }
            const QString overrideSummary = overrideDescriptions.isEmpty()
                ? QStringLiteral("无")
                : overrideDescriptions.join(QStringLiteral("，"));
            return QStringLiteral(
                       "模式：同心圆阵列\n"
                       "圆心：(%1, %2) mm\n"
                       "最大半径：%3 mm\n"
                       "圆环数量：%4\n"
                       "默认每圈点数：%5\n"
                       "单圈覆盖：%6")
                .arg(circle.centerX, 0, 'f', 3)
                .arg(circle.centerY, 0, 'f', 3)
                .arg(circle.maximumRadius, 0, 'f', 3)
                .arg(circle.ringCount)
                .arg(circle.defaultPointCount)
                .arg(overrideSummary);
        }
    case TaskMode::Csv:
        return QStringLiteral("模式：加载 CSV 文件\n文件：%1").arg(csv.filePath);
    }
    return {};
}

int CircleTaskConfiguration::pointCountForRing(int ringIndex) const
{
    for (const CircleRingPointOverride& pointOverride : ringPointOverrides) {
        if (pointOverride.ringIndex == ringIndex) {
            return pointOverride.pointCount;
        }
    }
    return defaultPointCount;
}
