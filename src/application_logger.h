#pragma once

#include <QString>

namespace otms {

bool initializeApplicationLogging(QString* errorMessage = nullptr);
void shutdownApplicationLogging();
[[nodiscard]] QString applicationLogFilePath();

} // namespace otms
