#include "motion_driver_factory.h"

#include "virtual_motion_driver.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(motionDriverFactoryLog, "otms.device.motion.factory")

namespace otms::device {

bool g_useVirtualMotionDriver = true;

std::shared_ptr<IMotionDriver> createMotionDriver(
    const QString& ipAddress,
    AgtioControllerType controllerType)
{
    if (g_useVirtualMotionDriver) {
        qCInfo(motionDriverFactoryLog) << "Using virtual motion driver";
        return std::make_shared<VirtualMotionDriver>();
    }

    qCInfo(motionDriverFactoryLog) << "Using AGTIO motion driver at" << ipAddress;
    return std::make_shared<AgtioMotionDriver>(ipAddress, controllerType);
}

} // namespace otms::device
