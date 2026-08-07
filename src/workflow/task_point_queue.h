#pragma once

#include "../config/task_configuration.h"

#include <optional>

#include <QList>
#include <QMetaType>
#include <QPointF>
#include <QString>

struct WorkpieceTaskPoint
{
    QPointF position;
    QString description;
};

[[nodiscard]] std::optional<QList<WorkpieceTaskPoint>> createWorkpiecePointQueue(
    const TaskConfiguration& configuration,
    QString* errorMessage = nullptr);

Q_DECLARE_METATYPE(WorkpieceTaskPoint)
Q_DECLARE_METATYPE(QList<WorkpieceTaskPoint>)
