#include "manticore/core/errors.h"
#include "manticore/core/time.h"

#include <chrono>
#include <sstream>

namespace manticore {

uint64_t monotonicTimeNs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

float samplesToMs(uint32_t sampleCount, uint32_t sampleRateHz)
{
    if (sampleRateHz == 0) {
        return 0.0f;
    }
    return static_cast<float>(sampleCount) / static_cast<float>(sampleRateHz) *
           1000.0f;
}

uint32_t msToSamples(float durationMs, uint32_t sampleRateHz)
{
    return static_cast<uint32_t>(durationMs * static_cast<float>(sampleRateHz) /
                                 1000.0f);
}

bool validateRuntimeConfig(const RuntimeConfig& config, std::string& errorMessage)
{
    if (config.sampleRateHz == 0) {
        errorMessage = "sampleRateHz must be > 0";
        return false;
    }
    if (config.channels == 0) {
        errorMessage = "channels must be >= 1";
        return false;
    }
    if (config.frameSize == 0) {
        errorMessage = "frameSize must be > 0";
        return false;
    }
    if (config.frameSize != 16 && config.frameSize != 32 &&
        config.frameSize != 64 && config.frameSize != 128) {
        std::ostringstream oss;
        oss << "frameSize " << config.frameSize
            << " not in supported set {16, 32, 64, 128}";
        errorMessage = oss.str();
        return false;
    }
    return true;
}

}  // namespace manticore
