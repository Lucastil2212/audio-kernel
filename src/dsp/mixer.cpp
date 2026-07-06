#include "manticore/dsp/mixer.h"

namespace manticore {

void mixFrames(const AudioFrame& a, const AudioFrame& b, AudioFrame& out,
               float gainA, float gainB)
{
    out.samples.resize(a.samples.size());
    out.sampleRateHz = a.sampleRateHz;
    out.channels = a.channels;
    out.frameIndex = a.frameIndex;
    out.timestampNs = a.timestampNs;

    for (size_t i = 0; i < a.samples.size(); ++i) {
        out.samples[i] = a.samples[i] * gainA + b.samples[i] * gainB;
    }
}

}  // namespace manticore
