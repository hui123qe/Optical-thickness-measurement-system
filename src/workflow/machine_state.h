#pragma once

namespace otms::workflow {

enum class MachineState
{
    NotReady,
    Ready,
    Running,
    Fault
};

enum class RunMode
{
    Manual,
    Automatic
};

} // namespace otms::workflow
