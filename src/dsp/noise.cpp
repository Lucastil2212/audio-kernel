#include "manticore/dsp/noise.h"

#include <limits>

namespace manticore {

namespace {
uint32_t lcgNext(uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    return state;
}
}

float WhiteNoiseGenerator::nextSample()
{
    const uint32_t value = lcgNext(state_);
    const float normalized =
        static_cast<float>(static_cast<int32_t>(value)) /
        static_cast<float>(std::numeric_limits<int32_t>::max());
    return normalized * amplitude;
}

void WhiteNoiseGenerator::render(AudioFrame& frame)
{
    for (size_t i = 0; i < frame.framesPerChannel(); ++i) {
        const float sample = nextSample();
        for (uint32_t ch = 0; ch < frame.channels; ++ch) {
            frame.set(i, ch, sample);
        }
    }
}

void PinkNoiseGenerator::reset()
{
    white_.reset();
    b0_ = b1_ = b2_ = b3_ = b4_ = b5_ = b6_ = 0.0f;
}

void PinkNoiseGenerator::render(AudioFrame& frame)
{
    white_.seed = seed;
    white_.amplitude = 1.0f;

    for (size_t i = 0; i < frame.framesPerChannel(); ++i) {
        const float white = white_.nextSample();
        b0_ = 0.99886f * b0_ + white * 0.0555179f;
        b1_ = 0.99332f * b1_ + white * 0.0750759f;
        b2_ = 0.96900f * b2_ + white * 0.1538520f;
        b3_ = 0.86650f * b3_ + white * 0.3104856f;
        b4_ = 0.55000f * b4_ + white * 0.5329522f;
        b5_ = -0.7616f * b5_ - white * 0.0168980f;
        const float pink = b0_ + b1_ + b2_ + b3_ + b4_ + b5_ + b6_ + white * 0.5362f;
        b6_ = white * 0.115926f;
        const float sample = pink * 0.11f * amplitude;
        for (uint32_t ch = 0; ch < frame.channels; ++ch) {
            frame.set(i, ch, sample);
        }
    }
}

}  // namespace manticore
