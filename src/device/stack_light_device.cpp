#include "stack_light_device.h"

#include "../motor/motion_controller_manager.h"

#include <QLoggingCategory>
#include <QString>

Q_LOGGING_CATEGORY(stackLightLog, "otms.device.stack_light")

namespace otms::device {

namespace {

constexpr const char* RedLampOutput = "redLamp";
constexpr const char* YellowLampOutput = "yellowLamp";
constexpr const char* GreenLampOutput = "greenLamp";

} // namespace

StackLightDevice::StackLightDevice(MotionControllerManager& motionControllers)
    : motionControllers_(motionControllers)
{
}

bool StackLightDevice::showNormal()
{
    qCInfo(stackLightLog) << "Setting stack light to normal";
    bool succeeded = writeOutput(RedLampOutput, false);
    succeeded = writeOutput(YellowLampOutput, false) && succeeded;
    succeeded = writeOutput(GreenLampOutput, true) && succeeded;
    return succeeded;
}

bool StackLightDevice::showFault()
{
    qCWarning(stackLightLog) << "Setting stack light to fault";
    bool succeeded = writeOutput(GreenLampOutput, false);
    succeeded = writeOutput(YellowLampOutput, false) && succeeded;
    succeeded = writeOutput(RedLampOutput, true) && succeeded;
    return succeeded;
}

bool StackLightDevice::turnOff()
{
    qCInfo(stackLightLog) << "Turning off stack light";
    bool succeeded = writeOutput(GreenLampOutput, false);
    succeeded = writeOutput(YellowLampOutput, false) && succeeded;
    succeeded = writeOutput(RedLampOutput, false) && succeeded;
    return succeeded;
}

bool StackLightDevice::writeOutput(const char* logicalOutput, bool enabled)
{
    const QString outputName = QString::fromLatin1(logicalOutput);
    qCInfo(stackLightLog) << "Writing stack light output" << outputName << enabled;
    if (motionControllers_.writeDigitalOutput(outputName, enabled) == 1) {
        return true;
    }

    qCCritical(stackLightLog) << "Failed to write stack light output" << outputName << enabled;
    return false;
}

} // namespace otms::device
