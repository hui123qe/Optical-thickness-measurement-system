#pragma once

#include <optional>

#include <QJsonObject>
#include <QList>
#include <QString>

struct CoordinateTransformConfiguration
{
    QString name;
    double rotationDegrees{};
    double translationX{};
    double translationY{};
};

class CoordinateTransformStore final
{
public:
    explicit CoordinateTransformStore(QString filePath = defaultFilePath());

    [[nodiscard]] QList<CoordinateTransformConfiguration> configurations(
        QString* errorMessage = nullptr) const;

    [[nodiscard]] static QString defaultFilePath();

private:
    [[nodiscard]] std::optional<QJsonObject> readRoot(QString* errorMessage) const;

    QString filePath_;
};
