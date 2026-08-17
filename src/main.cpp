#include "application_logger.h"
#include "device/device_manager.h"
#include "device/device_runtime_settings.h"
#include "main_window.h"
#include "motor/motion_controller_manager.h"
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

    QString motionInitializationError;
    otms::device::MotionControllerManager& motionControllers =
        otms::device::MotionControllerManager::instance();
    if (!motionControllers.initialize(&motionInitializationError)) {
        qWarning().noquote() << motionInitializationError;
    }

    otms::device::DeviceManager& devices = otms::device::DeviceManager::instance();
    if (otms::device::g_useVirtualLaserProbe) {
        std::uint8_t programNumber = 0;
        const otms::device::LaserStatus configurationStatus =
            devices.loadLaserConfiguration(programNumber);
        const otms::device::LaserStatus connectionStatus = configurationStatus.ok()
            ? devices.connectLaserEthernet()
            : configurationStatus;
        const otms::device::LaserStatus enableStatus = connectionStatus.ok()
            ? devices.enableLaser()
            : connectionStatus;
        if (!enableStatus.ok()) {
            qWarning().noquote() << enableStatus.message;
        }
    }

    MainWindow window;
    window.show();
    const int exitCode = application.exec();
    motionControllers.shutdown();
    otms::shutdownApplicationLogging();
    return exitCode;
}
