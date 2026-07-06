#pragma once

#include <cstdint>

namespace manticore {

struct RuntimeConfig {
    uint32_t sampleRateHz = 48000;
    uint32_t frameSize = 64;
    uint32_t channels = 1;
    bool enableLimiter = true;
    bool enableTelemetry = true;
    bool enableTherapy = false;
    float inputGain = 1.0f;
    float outputGain = 1.0f;
};

}  // namespace manticore
