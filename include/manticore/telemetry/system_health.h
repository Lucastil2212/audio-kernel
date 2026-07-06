#pragma once

#include <atomic>
#include <cstdint>

#include "manticore/safety/cue_health.h"

namespace manticore {

struct SystemHealth {
    uint64_t callbackCount = 0;
    uint64_t overrunCount = 0;
    uint64_t underflowCount = 0;
    uint64_t clipCount = 0;
    float lastPeakAbs = 0.0f;
    float lastRms = 0.0f;
    float cpuLoad = 0.0f;
    float callbackDurationMs = 0.0f;
    CueHealth cueHealth;
};

struct AtomicSystemHealth {
    std::atomic<uint64_t> callbackCount{0};
    std::atomic<uint64_t> overrunCount{0};
    std::atomic<uint64_t> underflowCount{0};
    std::atomic<uint64_t> clipCount{0};
    std::atomic<float> lastPeakAbs{0.0f};
    std::atomic<float> lastRms{0.0f};
    std::atomic<float> callbackDurationMs{0.0f};

    SystemHealth snapshot() const;
};

}  // namespace manticore
