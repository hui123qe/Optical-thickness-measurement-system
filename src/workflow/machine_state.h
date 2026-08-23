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

enum class InitializationState
{
    NotStarted,
    Running,
    Completed,
    Failed
};

} // namespace otms::workflow
