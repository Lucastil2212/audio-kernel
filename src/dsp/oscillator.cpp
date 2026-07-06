#include "manticore/dsp/oscillator.h"

namespace manticore {

namespace {
constexpr float kTwoPi = 6.283185307179586f;
}

void SineOscillator::render(AudioFrame& frame)
{
    if (frame.sampleRateHz == 0) {
        return;
    }

    const float phaseIncrement =
        kTwoPi * frequencyHz / static_cast<float>(frame.sampleRateHz);

    for (size_t i = 0; i < frame.framesPerChannel(); ++i) {
        for (uint32_t ch = 0; ch < frame.channels; ++ch) {
            frame.set(i, ch, amplitude * std::sin(phaseRadians));
        }
        phaseRadians += phaseIncrement;
        if (phaseRadians >= kTwoPi) {
            phaseRadians -= kTwoPi;
        }
    }
}

}  // namespace manticore
