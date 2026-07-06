#include "manticore/telemetry/system_health.h"

namespace manticore {

SystemHealth AtomicSystemHealth::snapshot() const
{
    SystemHealth health;
    health.callbackCount = callbackCount.load(std::memory_order_relaxed);
    health.overrunCount = overrunCount.load(std::memory_order_relaxed);
    health.underflowCount = underflowCount.load(std::memory_order_relaxed);
    health.clipCount = clipCount.load(std::memory_order_relaxed);
    health.lastPeakAbs = lastPeakAbs.load(std::memory_order_relaxed);
    health.lastRms = lastRms.load(std::memory_order_relaxed);
    health.callbackDurationMs =
        callbackDurationMs.load(std::memory_order_relaxed);
    return health;
}

}  // namespace manticore
