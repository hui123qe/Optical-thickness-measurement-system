#include "task_configuration_store.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <cmath>
#include <limits>
#include <utility>

namespace {

constexpr int configurationSchemaVersion = 4;

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

QJsonObject pointToJson(const QPointF& point)
{
    QJsonObject object;
    object.insert(QStringLiteral("x"), point.x());
    object.insert(QStringLiteral("y"), point.y());
    return object;
}

bool readPoint(
    const QJsonObject& object,
    const QString& name,
    QPointF& point,
    QString* errorMessage)
{
    const QJsonValue value = object.value(name);
    if (!value.isObject()) {
        setError(errorMessage, QStringLiteral("矩形配置缺少 %1 角点。").arg(name));
        return false;
    }

    const QJsonObject pointObject = value.toObject();
    const QJsonValue x = pointObject.value(QStringLiteral("x"));
    const QJsonValue y = pointObject.value(QStringLiteral("y"));
    if (!x.isDouble() || !y.isDouble()) {
        setError(errorMessage, QStringLiteral("矩形配置的 %1 角点坐标无效。").arg(name));
        return false;
    }

    point = QPointF(x.toDouble(), y.toDouble());
    return true;
}

bool readPositiveInteger(
    const QJsonObject& object,
    const QString& name,
    int& result,
    QString* errorMessage)
{
    const QJsonValue value = object.value(name);
    const double number = value.toDouble();
    if (!value.isDouble() || number < 1.0 || number != std::floor(number)
        || number > static_cast<double>(std::numeric_limits<int>::max())) {
        setError(errorMessage, QStringLiteral("配置字段“%1”必须是正整数。").arg(name));
        return false;
    }

    result = static_cast<int>(number);
    return true;
}

QString modeToString(TaskMode mode)
{
    switch (mode) {
    case TaskMode::Rectangle:
        return QStringLiteral("rectangle");
    case TaskMode::Circle:
        return QStringLiteral("circle");
    case TaskMode::Csv:
        return QStringLiteral("csv");
    }
    return {};
}

std::optional<TaskMode> modeFromString(const QString& value)
{
    if (value == QStringLiteral("rectangle")) {
        return TaskMode::Rectangle;
    }
    if (value == QStringLiteral("circle")) {
        return TaskMode::Circle;
    }
    if (value == QStringLiteral("csv")) {
        return TaskMode::Csv;
    }
    return std::nullopt;
}

QJsonObject toJson(const TaskConfiguration& configuration)
{
    QJsonObject object;
    object.insert(QStringLiteral("mode"), modeToString(configuration.mode));

    switch (configuration.mode) {
    case TaskMode::Rectangle: {
        QJsonObject rectangle;
        for (std::size_t index = 0; index < configuration.rectangle.corners.size(); ++index) {
            rectangle.insert(
                QStringLiteral("corner%1").arg(static_cast<int>(index + 1)),
                pointToJson(configuration.rectangle.corners[index]));
        }
        rectangle.insert(QStringLiteral("rowCount"), configuration.rectangle.rowCount);
        rectangle.insert(QStringLiteral("columnCount"), configuration.rectangle.columnCount);
        object.insert(QStringLiteral("rectangle"), rectangle);
        break;
    }
    case TaskMode::Circle: {
        QJsonObject circle;
        circle.insert(QStringLiteral("centerX"), configuration.circle.centerX);
        circle.insert(QStringLiteral("centerY"), configuration.circle.centerY);
        circle.insert(QStringLiteral("maximumRadius"), configuration.circle.maximumRadius);
        circle.insert(QStringLiteral("ringCount"), configuration.circle.ringCount);
        circle.insert(QStringLiteral("defaultPointCount"), configuration.circle.defaultPointCount);
        QJsonArray ringPointOverrides;
        for (const CircleRingPointOverride& pointOverride : configuration.circle.ringPointOverrides) {
            QJsonObject overrideObject;
            overrideObject.insert(QStringLiteral("ringIndex"), pointOverride.ringIndex);
            overrideObject.insert(QStringLiteral("pointCount"), pointOverride.pointCount);
            ringPointOverrides.append(overrideObject);
        }
        circle.insert(QStringLiteral("ringPointOverrides"), ringPointOverrides);
        object.insert(QStringLiteral("circle"), circle);
        break;
    }
    case TaskMode::Csv: {
        QJsonObject csv;
        csv.insert(QStringLiteral("filePath"), configuration.csv.filePath);
        object.insert(QStringLiteral("csv"), csv);
        break;
    }
    }
    return object;
}

std::optional<TaskConfiguration> fromJson(const QJsonObject& object, QString* errorMessage)
{
    const std::optional<TaskMode> mode = modeFromString(object.value(QStringLiteral("mode")).toString());
    if (!mode.has_value()) {
        setError(errorMessage, QStringLiteral("配置中的任务模式无效。"));
        return std::nullopt;
    }

    TaskConfiguration configuration;
    configuration.mode = *mode;

    switch (*mode) {
    case TaskMode::Rectangle: {
        const QJsonObject rectangle = object.value(QStringLiteral("rectangle")).toObject();
        if (rectangle.isEmpty()) {
            setError(errorMessage, QStringLiteral("矩形配置内容不完整。"));
            return std::nullopt;
        }
        for (std::size_t index = 0; index < configuration.rectangle.corners.size(); ++index) {
            if (!readPoint(
                    rectangle,
                    QStringLiteral("corner%1").arg(static_cast<int>(index + 1)),
                    configuration.rectangle.corners[index],
                    errorMessage)) {
                return std::nullopt;
            }
        }
        if (!readPositiveInteger(
                rectangle,
                QStringLiteral("rowCount"),
                configuration.rectangle.rowCount,
                errorMessage)
            || !readPositiveInteger(
                rectangle,
                QStringLiteral("columnCount"),
                configuration.rectangle.columnCount,
                errorMessage)) {
            return std::nullopt;
        }
        break;
    }
    case TaskMode::Circle: {
        const QJsonObject circle = object.value(QStringLiteral("circle")).toObject();
        if (circle.isEmpty()) {
            setError(errorMessage, QStringLiteral("圆形配置内容不完整。"));
            return std::nullopt;
        }
        const QJsonValue centerX = circle.value(QStringLiteral("centerX"));
        const QJsonValue centerY = circle.value(QStringLiteral("centerY"));
        const QJsonValue maximumRadius = circle.value(QStringLiteral("maximumRadius"));
        const QJsonValue overrideValue = circle.value(QStringLiteral("ringPointOverrides"));
        if (!centerX.isDouble() || !centerY.isDouble() || !maximumRadius.isDouble()
            || !overrideValue.isArray()) {
            setError(errorMessage, QStringLiteral("圆形配置内容不完整或字段类型无效。"));
            return std::nullopt;
        }
        configuration.circle.centerX = centerX.toDouble();
        configuration.circle.centerY = centerY.toDouble();
        configuration.circle.maximumRadius = maximumRadius.toDouble();
        if (!readPositiveInteger(
                circle,
                QStringLiteral("ringCount"),
                configuration.circle.ringCount,
                errorMessage)
            || !readPositiveInteger(
                circle,
                QStringLiteral("defaultPointCount"),
                configuration.circle.defaultPointCount,
                errorMessage)) {
            return std::nullopt;
        }
        const QJsonArray overrideArray = overrideValue.toArray();
        configuration.circle.ringPointOverrides.clear();
        configuration.circle.ringPointOverrides.reserve(overrideArray.size());
        for (qsizetype index = 0; index < overrideArray.size(); ++index) {
            const QJsonObject overrideObject = overrideArray.at(index).toObject();
            CircleRingPointOverride pointOverride;
            if (overrideObject.isEmpty()
                || !readPositiveInteger(
                    overrideObject,
                    QStringLiteral("ringIndex"),
                    pointOverride.ringIndex,
                    errorMessage)
                || !readPositiveInteger(
                    overrideObject,
                    QStringLiteral("pointCount"),
                    pointOverride.pointCount,
                    errorMessage)) {
                setError(
                    errorMessage,
                    QStringLiteral("第 %1 个圆环点数覆盖配置无效。")
                        .arg(static_cast<qlonglong>(index + 1)));
                return std::nullopt;
            }
            configuration.circle.ringPointOverrides.append(pointOverride);
        }
        break;
    }
    case TaskMode::Csv: {
        const QJsonObject csv = object.value(QStringLiteral("csv")).toObject();
        if (csv.isEmpty()) {
            setError(errorMessage, QStringLiteral("CSV 配置内容不完整。"));
            return std::nullopt;
        }
        configuration.csv.filePath = csv.value(QStringLiteral("filePath")).toString();
        break;
    }
    }
    return configuration;
}

} // namespace

TaskConfigurationStore::TaskConfigurationStore(QString filePath)
    : filePath_(std::move(filePath))
{
}

QStringList TaskConfigurationStore::configurationNames(QString* errorMessage) const
{
    const std::optional<QJsonObject> root = readRoot(errorMessage);
    if (!root.has_value()) {
        return {};
    }

    QStringList names = root->value(QStringLiteral("configurations")).toObject().keys();
    names.sort(Qt::CaseInsensitive);
    return names;
}

std::optional<TaskConfiguration> TaskConfigurationStore::load(const QString& name, QString* errorMessage) const
{
    const std::optional<QJsonObject> root = readRoot(errorMessage);
    if (!root.has_value()) {
        return std::nullopt;
    }

    const QJsonObject configurations = root->value(QStringLiteral("configurations")).toObject();
    if (!configurations.contains(name) || !configurations.value(name).isObject()) {
        setError(errorMessage, QStringLiteral("未找到配置“%1”。").arg(name));
        return std::nullopt;
    }
    return fromJson(configurations.value(name).toObject(), errorMessage);
}

bool TaskConfigurationStore::save(const QString& name, const TaskConfiguration& configuration, QString* errorMessage) const
{
    std::optional<QJsonObject> root = readRoot(errorMessage);
    if (!root.has_value()) {
        return false;
    }

    QJsonObject configurations = root->value(QStringLiteral("configurations")).toObject();
    configurations.insert(name, toJson(configuration));
    root->insert(QStringLiteral("configurations"), configurations);
    return writeRoot(*root, errorMessage);
}

QString TaskConfigurationStore::defaultFilePath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("task-configurations-v4.json"));
}

std::optional<QJsonObject> TaskConfigurationStore::readRoot(QString* errorMessage) const
{
    QFile file(filePath_);
    if (!file.exists()) {
        QJsonObject root;
        root.insert(QStringLiteral("schemaVersion"), configurationSchemaVersion);
        root.insert(QStringLiteral("configurations"), QJsonObject{});
        return root;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("无法读取配置文件：%1").arg(file.errorString()));
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QStringLiteral("配置文件格式错误：%1").arg(parseError.errorString()));
        return std::nullopt;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("schemaVersion")).toInt() != configurationSchemaVersion
        || !root.value(QStringLiteral("configurations")).isObject()) {
        setError(errorMessage, QStringLiteral("配置文件版本不受支持或内容不完整。"));
        return std::nullopt;
    }
    return root;
}

bool TaskConfigurationStore::writeRoot(const QJsonObject& root, QString* errorMessage) const
{
    const QFileInfo fileInfo(filePath_);
    QDir directory(fileInfo.absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        setError(errorMessage, QStringLiteral("无法创建配置目录：%1").arg(directory.absolutePath()));
        return false;
    }

    QSaveFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(errorMessage, QStringLiteral("无法写入配置文件：%1").arg(file.errorString()));
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        setError(errorMessage, QStringLiteral("写入配置文件失败：%1").arg(file.errorString()));
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        setError(errorMessage, QStringLiteral("保存配置文件失败：%1").arg(file.errorString()));
        return false;
    }
    return true;
}
