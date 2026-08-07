#pragma once

#include <array>
#include <optional>

#include <QList>
#include <QPointF>
#include <QString>

enum class TaskMode
{
    Rectangle,
    Circle,
    Csv
};

struct RectangleGridCorners
{
    QPointF origin;
    QPointF columnEnd;
    QPointF rowEnd;
    QPointF opposite;
};

struct RectangleTaskConfiguration
{
    std::array<QPointF, 4> corners{
        QPointF{-10.0, -5.0},
        QPointF{10.0, -5.0},
        QPointF{10.0, 5.0},
        QPointF{-10.0, 5.0}};
    int rowCount{3};
    int columnCount{4};

    [[nodiscard]] std::optional<RectangleGridCorners> normalizedGridCorners(
        QString* errorMessage = nullptr) const;
};

struct CircleRingPointOverride
{
    int ringIndex{};
    int pointCount{12};
};

struct CircleTaskConfiguration
{
    double centerX{};
    double centerY{};
    double maximumRadius{10.0};
    int ringCount{2};
    int defaultPointCount{12};
    QList<CircleRingPointOverride> ringPointOverrides;

    [[nodiscard]] int pointCountForRing(int ringIndex) const;
};

struct CsvTaskConfiguration
{
    QString filePath;

    [[nodiscard]] QString resolvedFilePath() const;
};

struct TaskConfiguration
{
    TaskMode mode{TaskMode::Rectangle};
    RectangleTaskConfiguration rectangle;
    CircleTaskConfiguration circle;
    CsvTaskConfiguration csv;

    [[nodiscard]] QString validationError() const;
    [[nodiscard]] QString modeName() const;
    [[nodiscard]] QString summary() const;
};
