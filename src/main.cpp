#include "application_logger.h"
#include "main_window.h"
#include "view/application_style.h"

#include <QApplication>
#include <QFont>
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QString loggingError;
    if (!otms::initializeApplicationLogging(&loggingError)) {
        qWarning().noquote() << loggingError;
    }
    application.setApplicationName(QStringLiteral("光学厚度测量系统"));
    application.setApplicationVersion(QStringLiteral("0.1.0"));
    application.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
    applyApplicationStyle();

    MainWindow window;
    window.show();
    const int exitCode = application.exec();
    otms::shutdownApplicationLogging();
    return exitCode;
}
