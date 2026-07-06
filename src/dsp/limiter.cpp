#include "manticore/dsp/limiter.h"

#include <cmath>

namespace manticore {

float clampSample(float sample, float minValue, float maxValue)
{
    if (sample > maxValue) {
        return maxValue;
    }
    if (sample < minValue) {
        return minValue;
    }
    return sample;
}

void applyLimiter(AudioFrame& frame, float threshold)
{
    const float negThreshold = -threshold;
    for (float& sample : frame.samples) {
        const float clamped = clampSample(sample, negThreshold, threshold);
        if (clamped != sample) {
            frame.clipFlag = true;
            sample = clamped;
        }
    }
}

}  // namespace manticore
