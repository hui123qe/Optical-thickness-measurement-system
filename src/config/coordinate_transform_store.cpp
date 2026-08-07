#include "coordinate_transform_store.h"

#include <cmath>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace {

constexpr int coordinateTransformSchemaVersion = 1;

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

} // namespace

CoordinateTransformStore::CoordinateTransformStore(QString filePath)
    : filePath_(std::move(filePath))
{
}

QList<CoordinateTransformConfiguration> CoordinateTransformStore::configurations(
    QString* errorMessage) const
{
    const std::optional<QJsonObject> root = readRoot(errorMessage);
    if (!root.has_value()) {
        return {};
    }

    const QJsonArray transformArray = root->value(QStringLiteral("coordinateTransforms")).toArray();
    if (transformArray.isEmpty()) {
        setError(errorMessage, QStringLiteral("坐标转换配置文件中没有可用方案。"));
        return {};
    }

    QList<CoordinateTransformConfiguration> transforms;
    transforms.reserve(transformArray.size());
    for (qsizetype index = 0; index < transformArray.size(); ++index) {
        const QJsonObject object = transformArray.at(index).toObject();
        const QString name = object.value(QStringLiteral("name")).toString().trimmed();
        const QJsonValue rotation = object.value(QStringLiteral("rotationDegrees"));
        const QJsonValue translationX = object.value(QStringLiteral("translationX"));
        const QJsonValue translationY = object.value(QStringLiteral("translationY"));
        if (name.isEmpty() || !rotation.isDouble() || !translationX.isDouble()
            || !translationY.isDouble() || !std::isfinite(rotation.toDouble())
            || !std::isfinite(translationX.toDouble())
            || !std::isfinite(translationY.toDouble())) {
            setError(
                errorMessage,
                QStringLiteral("第 %1 个坐标转换方案内容不完整或数值无效。")
                    .arg(static_cast<qlonglong>(index + 1)));
            return {};
        }

        transforms.append(CoordinateTransformConfiguration{
            name,
            rotation.toDouble(),
            translationX.toDouble(),
            translationY.toDouble()
        });
    }
    return transforms;
}

QString CoordinateTransformStore::defaultFilePath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("coordinate-transforms.json"));
}

std::optional<QJsonObject> CoordinateTransformStore::readRoot(QString* errorMessage) const
{
    QFile file(filePath_);
    if (!file.exists()) {
        setError(errorMessage, QStringLiteral("坐标转换配置文件不存在：%1").arg(filePath_));
        return std::nullopt;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("无法读取坐标转换配置文件：%1").arg(file.errorString()));
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(
            errorMessage,
            QStringLiteral("坐标转换配置文件格式错误：%1").arg(parseError.errorString()));
        return std::nullopt;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schemaVersion")).toInt()
            != coordinateTransformSchemaVersion
        || !root.value(QStringLiteral("coordinateTransforms")).isArray()) {
        setError(errorMessage, QStringLiteral("坐标转换配置文件版本不受支持或内容不完整。"));
        return std::nullopt;
    }
    return root;
}
