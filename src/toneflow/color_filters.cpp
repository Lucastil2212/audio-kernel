#include "manticore/toneflow/color_filters.h"

#include <cmath>

namespace manticore::toneflow {
namespace {
constexpr float kPi = 3.14159265358979323846f;
}

float ColorFilterBank::Biquad::process(float x)
{
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

void ColorFilterBank::Biquad::reset()
{
    z1 = z2 = 0.0f;
}

void ColorFilterBank::Biquad::setLowPass(float freq, float q, float sr)
{
    const float w0 = 2.0f * kPi * clamp(freq, 20.0f, sr * 0.45f) / sr;
    const float alpha = std::sin(w0) / (2.0f * q);
    const float cosw = std::cos(w0);
    const float a0 = 1.0f + alpha;
    b0 = ((1.0f - cosw) * 0.5f) / a0;
    b1 = (1.0f - cosw) / a0;
    b2 = b0;
    a1 = (-2.0f * cosw) / a0;
    a2 = (1.0f - alpha) / a0;
}

void ColorFilterBank::Biquad::setHighPass(float freq, float q, float sr)
{
    const float w0 = 2.0f * kPi * clamp(freq, 20.0f, sr * 0.45f) / sr;
    const float alpha = std::sin(w0) / (2.0f * q);
    const float cosw = std::cos(w0);
    const float a0 = 1.0f + alpha;
    b0 = ((1.0f + cosw) * 0.5f) / a0;
    b1 = (-(1.0f + cosw)) / a0;
    b2 = b0;
    a1 = (-2.0f * cosw) / a0;
    a2 = (1.0f - alpha) / a0;
}

void ColorFilterBank::Biquad::setBandPass(float freq, float q, float sr)
{
    const float w0 = 2.0f * kPi * clamp(freq, 20.0f, sr * 0.45f) / sr;
    const float alpha = std::sin(w0) / (2.0f * q);
    const float cosw = std::cos(w0);
    const float a0 = 1.0f + alpha;
    b0 = alpha / a0;
    b1 = 0.0f;
    b2 = -alpha / a0;
    a1 = (-2.0f * cosw) / a0;
    a2 = (1.0f - alpha) / a0;
}

void ColorFilterBank::Biquad::setNotch(float freq, float q, float sr)
{
    const float w0 = 2.0f * kPi * clamp(freq, 20.0f, sr * 0.45f) / sr;
    const float alpha = std::sin(w0) / (2.0f * q);
    const float cosw = std::cos(w0);
    const float a0 = 1.0f + alpha;
    b0 = 1.0f / a0;
    b1 = (-2.0f * cosw) / a0;
    b2 = 1.0f / a0;
    a1 = (-2.0f * cosw) / a0;
    a2 = (1.0f - alpha) / a0;
}

void ColorFilterBank::Biquad::setPeaking(float freq, float q, float gainDb, float sr)
{
    const float A = std::pow(10.0f, gainDb / 40.0f);
    const float w0 = 2.0f * kPi * clamp(freq, 20.0f, sr * 0.45f) / sr;
    const float alpha = std::sin(w0) / (2.0f * q);
    const float cosw = std::cos(w0);
    const float a0 = 1.0f + alpha / A;
    b0 = (1.0f + alpha * A) / a0;
    b1 = (-2.0f * cosw) / a0;
    b2 = (1.0f - alpha * A) / a0;
    a1 = (-2.0f * cosw) / a0;
    a2 = (1.0f - alpha / A) / a0;
}

void ColorFilterBank::prepare(uint32_t sampleRateHz)
{
    sampleRateHz_ = sampleRateHz == 0 ? 48000 : sampleRateHz;
    rebuild();
}

void ColorFilterBank::setType(FilterType type)
{
    if (type_ != type) {
        type_ = type;
        rebuild();
        reset();
    }
}

void ColorFilterBank::setToneHz(float centerHz)
{
    const float c = clamp(centerHz, 100.0f, 1000.0f);
    if (std::fabs(c - toneHz_) > 2.0f) {
        toneHz_ = c;
        if (type_ == FilterType::SoftBand || type_ == FilterType::Presence) {
            rebuild();
        }
    }
}

void ColorFilterBank::rebuild()
{
    const float sr = static_cast<float>(sampleRateHz_);
    // Bypass identity
    leftA_ = Biquad{};
    leftB_ = Biquad{};
    rightA_ = Biquad{};
    rightB_ = Biquad{};

    switch (type_) {
        case FilterType::Warm:
            leftA_.setLowPass(2200.0f, 0.707f, sr);
            rightA_.setLowPass(2200.0f, 0.707f, sr);
            leftB_.setHighPass(60.0f, 0.707f, sr);
            rightB_.setHighPass(60.0f, 0.707f, sr);
            break;
        case FilterType::Presence:
            leftA_.setPeaking(std::max(toneHz_ * 1.5f, 400.0f), 1.0f, 2.5f, sr);
            rightA_.setPeaking(std::max(toneHz_ * 1.5f, 400.0f), 1.0f, 2.5f, sr);
            leftB_.setLowPass(5000.0f, 0.7f, sr);
            rightB_.setLowPass(5000.0f, 0.7f, sr);
            break;
        case FilterType::SoftBand:
            leftA_.setBandPass(toneHz_, 1.4f, sr);
            rightA_.setBandPass(toneHz_, 1.4f, sr);
            leftB_.setLowPass(toneHz_ * 4.0f, 0.7f, sr);
            rightB_.setLowPass(toneHz_ * 4.0f, 0.7f, sr);
            break;
        case FilterType::Crystal:
            leftA_.setHighPass(180.0f, 0.8f, sr);
            rightA_.setHighPass(180.0f, 0.8f, sr);
            leftB_.setPeaking(3200.0f, 0.9f, 1.8f, sr);
            rightB_.setPeaking(3200.0f, 0.9f, 1.8f, sr);
            break;
        case FilterType::DeepWell:
            leftA_.setLowPass(900.0f, 0.9f, sr);
            rightA_.setLowPass(900.0f, 0.9f, sr);
            leftB_.setHighPass(40.0f, 0.7f, sr);
            rightB_.setHighPass(40.0f, 0.7f, sr);
            break;
        case FilterType::NotchHum:
            leftA_.setNotch(60.0f, 8.0f, sr);
            rightA_.setNotch(60.0f, 8.0f, sr);
            leftB_.setLowPass(2800.0f, 0.7f, sr);
            rightB_.setLowPass(2800.0f, 0.7f, sr);
            break;
        default:
            break;
    }
}

void ColorFilterBank::reset()
{
    leftA_.reset();
    leftB_.reset();
    rightA_.reset();
    rightB_.reset();
}

void ColorFilterBank::process(AudioFrame& stereo)
{
    if (type_ == FilterType::Off || !stereo.isStereo()) {
        return;
    }
    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        float l = stereo.get(i, 0);
        float r = stereo.get(i, 1);
        l = leftB_.process(leftA_.process(l));
        r = rightB_.process(rightA_.process(r));
        stereo.set(i, 0, l);
        stereo.set(i, 1, r);
    }
}

}  // namespace manticore::toneflow
