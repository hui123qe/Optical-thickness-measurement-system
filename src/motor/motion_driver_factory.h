#pragma once

#include "agtio_motion_driver.h"
#include "motion_driver.h"

#include <memory>

#include <QString>

namespace otms::device {

// Set this before calling createMotionDriver().
extern bool g_useVirtualMotionDriver;

std::shared_ptr<IMotionDriver> createMotionDriver(
    const QString& ipAddress,
    AgtioControllerType controllerType);

} // namespace otms::device
