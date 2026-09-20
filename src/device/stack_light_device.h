#pragma once

namespace otms::device {

class MotionControllerManager;

class StackLightDevice final
{
public:
    explicit StackLightDevice(MotionControllerManager& motionControllers);

    bool showNormal();
    bool showFault();
    bool turnOff();

private:
    bool writeOutput(const char* logicalOutput, bool enabled);

    MotionControllerManager& motionControllers_;
};

} // namespace otms::device
