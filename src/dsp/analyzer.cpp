#include "manticore/dsp/analyzer.h"

#include <cmath>
#include <limits>

namespace manticore {

FrameStats analyzeFrame(const AudioFrame& frame)
{
    FrameStats stats;
    if (frame.samples.empty()) {
        return stats;
    }

    stats.minSample = std::numeric_limits<float>::max();
    stats.maxSample = std::numeric_limits<float>::lowest();
    double sum = 0.0;
    double sumSquares = 0.0;

    for (float sample : frame.samples) {
        if (std::isnan(sample)) {
            stats.hasNaN = true;
        }
        if (std::isinf(sample)) {
            stats.hasInf = true;
        }

        if (sample < stats.minSample) {
            stats.minSample = sample;
        }
        if (sample > stats.maxSample) {
            stats.maxSample = sample;
        }

        const float absSample = std::fabs(sample);
        if (absSample > stats.peakAbs) {
            stats.peakAbs = absSample;
        }

        if (sample >= 1.0f || sample <= -1.0f) {
            stats.clipped = true;
        }

        sum += static_cast<double>(sample);
        sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
    }

    const double count = static_cast<double>(frame.samples.size());
    stats.dcOffset = static_cast<float>(sum / count);
    stats.rms = static_cast<float>(std::sqrt(sumSquares / count));

    return stats;
}

}  // namespace manticore
