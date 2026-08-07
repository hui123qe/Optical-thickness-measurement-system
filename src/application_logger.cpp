#include "application_logger.h"

#include <cstdio>
#include <mutex>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageLogContext>
#include <QThread>

namespace otms {
namespace {

std::mutex logMutex;
QFile logFile;
QtMessageHandler previousMessageHandler = nullptr;

const char* messageLevel(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return "DEBUG";
    case QtInfoMsg:
        return "INFO";
    case QtWarningMsg:
        return "WARN";
    case QtCriticalMsg:
        return "ERROR";
    case QtFatalMsg:
        return "FATAL";
    }
    return "UNKNOWN";
}

void applicationMessageHandler(
    QtMsgType type,
    const QMessageLogContext& context,
    const QString& message)
{
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    const quintptr threadId = reinterpret_cast<quintptr>(QThread::currentThreadId());
    const QString category = context.category != nullptr
        ? QString::fromUtf8(context.category)
        : QStringLiteral("default");
    const QString source = context.file != nullptr
        ? QStringLiteral("%1:%2")
              .arg(QFileInfo(QString::fromUtf8(context.file)).fileName())
              .arg(context.line)
        : QStringLiteral("unknown:0");
    const QByteArray line = QStringLiteral(
                                "%1 [%2] [thread=0x%3] [%4] [%5] %6\n")
                                .arg(
                                    timestamp,
                                    QString::fromLatin1(messageLevel(type)),
                                    QString::number(threadId, 16),
                                    source,
                                    category,
                                    message)
                                .toUtf8();

    {
        std::scoped_lock lock(logMutex);
        if (logFile.isOpen()) {
            logFile.write(line);
            logFile.flush();
        }
    }

    if (previousMessageHandler != nullptr) {
        previousMessageHandler(type, context, message);
    } else {
        std::fwrite(line.constData(), 1, static_cast<std::size_t>(line.size()), stderr);
        std::fflush(stderr);
    }
}

} // namespace

bool initializeApplicationLogging(QString* errorMessage)
{
    const QString filePath = applicationLogFilePath();
    const QFileInfo fileInfo(filePath);
    QDir directory;
    if (!directory.mkpath(fileInfo.absolutePath())) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法创建运行日志目录：%1").arg(fileInfo.absolutePath());
        }
        return false;
    }

    logFile.setFileName(filePath);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("无法创建运行日志：%1").arg(logFile.errorString());
        }
        return false;
    }

    previousMessageHandler = qInstallMessageHandler(applicationMessageHandler);
    qInfo().noquote() << QStringLiteral("应用程序启动，运行日志已重置：%1").arg(filePath);
    return true;
}

void shutdownApplicationLogging()
{
    qInfo() << "Application shutdown";
    qInstallMessageHandler(previousMessageHandler);
    previousMessageHandler = nullptr;
    std::scoped_lock lock(logMutex);
    logFile.close();
}

QString applicationLogFilePath()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("logs/runtime.log"));
}

} // namespace otms
