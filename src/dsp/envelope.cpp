#include "manticore/dsp/envelope.h"

namespace manticore {

namespace {
constexpr float kTwoPi = 6.283185307179586f;
}

void SineEnvelope::set(float rateHz, float depth, float sampleRateHz)
{
    rateHz_ = rateHz;
    depth_ = depth;
    sampleRateHz_ = sampleRateHz;
}

float SineEnvelope::next()
{
    const float envelope =
        1.0f - depth_ + depth_ * 0.5f * (1.0f + std::sin(phaseRadians_));
    const float phaseIncrement = kTwoPi * rateHz_ / sampleRateHz_;
    phaseRadians_ += phaseIncrement;
    if (phaseRadians_ >= kTwoPi) {
        phaseRadians_ -= kTwoPi;
    }
    return envelope;
}

void SineEnvelope::reset() { phaseRadians_ = 0.0f; }

}  // namespace manticore
