#pragma once

#include "task_configuration.h"

#include <optional>

#include <QJsonObject>
#include <QString>
#include <QStringList>

class TaskConfigurationStore final
{
public:
    explicit TaskConfigurationStore(QString filePath = defaultFilePath());

    [[nodiscard]] QStringList configurationNames(QString* errorMessage = nullptr) const;
    [[nodiscard]] std::optional<TaskConfiguration> load(const QString& name, QString* errorMessage = nullptr) const;
    bool save(const QString& name, const TaskConfiguration& configuration, QString* errorMessage = nullptr) const;

    [[nodiscard]] static QString defaultFilePath();

private:
    [[nodiscard]] std::optional<QJsonObject> readRoot(QString* errorMessage) const;
    bool writeRoot(const QJsonObject& root, QString* errorMessage) const;

    QString filePath_;
};
