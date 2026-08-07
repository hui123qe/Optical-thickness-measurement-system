#include "application_style.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QString>

void applyApplicationStyle()
{
    QFile styleFile(QStringLiteral(":/styles/application.qss"));
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning().noquote() << QStringLiteral("无法加载应用样式资源：%1").arg(styleFile.errorString());
        return;
    }

    qApp->setStyleSheet(QString::fromUtf8(styleFile.readAll()));
}
