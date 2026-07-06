#include "manticore/dsp/filters.h"

#include <cmath>

namespace manticore {

namespace {
constexpr float kTwoPi = 6.283185307179586f;
}

void OnePoleLowPass::setCutoff(float cutoffHz, float sampleRateHz)
{
    const float dt = 1.0f / sampleRateHz;
    const float rc = 1.0f / (kTwoPi * cutoffHz);
    alpha_ = dt / (rc + dt);
}

void OnePoleLowPass::reset() { z1_ = 0.0f; }

float OnePoleLowPass::processSample(float x)
{
    z1_ = z1_ + alpha_ * (x - z1_);
    return z1_;
}

void OnePoleLowPass::processFrame(AudioFrame& frame)
{
    for (float& sample : frame.samples) {
        sample = processSample(sample);
    }
}

void OnePoleHighPass::setCutoff(float cutoffHz, float sampleRateHz)
{
    const float dt = 1.0f / sampleRateHz;
    const float rc = 1.0f / (kTwoPi * cutoffHz);
    alpha_ = rc / (rc + dt);
}

void OnePoleHighPass::reset()
{
    previousInput_ = 0.0f;
    previousOutput_ = 0.0f;
}

float OnePoleHighPass::processSample(float x)
{
    const float y = alpha_ * (previousOutput_ + x - previousInput_);
    previousInput_ = x;
    previousOutput_ = y;
    return y;
}

void OnePoleHighPass::processFrame(AudioFrame& frame)
{
    for (float& sample : frame.samples) {
        sample = processSample(sample);
    }
}

}  // namespace manticore
