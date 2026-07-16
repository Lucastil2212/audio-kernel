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
    rebuildBloom();
    reset();
}

void ColorFilterBank::setType(FilterType type)
{
    if (type_ != type) {
        type_ = type;
        rebuild();
        // Keep running state — only zero when explicitly reset().
    }
}

void ColorFilterBank::setToneHz(float centerHz)
{
    const float c = clamp(centerHz, 100.0f, 1000.0f);
    if (std::fabs(c - toneHz_) > 0.5f) {
        toneHz_ = c;
        rebuildBloom();
        if (type_ == FilterType::SoftBand || type_ == FilterType::Presence ||
            type_ == FilterType::Warm || type_ == FilterType::DeepWell ||
            type_ == FilterType::Crystal) {
            rebuild();
        }
    }
}

void ColorFilterBank::rebuildBloom()
{
    const float sr = static_cast<float>(sampleRateHz_);
    // Velvet low-pass only — no ringing peaks (those sounded staticy).
    bloomL_.setLowPass(2400.0f, 0.707f, sr);
    bloomR_.setLowPass(2400.0f, 0.707f, sr);
}

void ColorFilterBank::rebuild()
{
    // Update coefficients only — never wipe z1/z2 (that caused zipper/static).
    const float sr = static_cast<float>(sampleRateHz_);

    switch (type_) {
        case FilterType::Warm:
            leftA_.setLowPass(2800.0f, 0.707f, sr);
            rightA_.setLowPass(2800.0f, 0.707f, sr);
            leftB_.setPeaking(toneHz_, 0.85f, 1.2f, sr);
            rightB_.setPeaking(toneHz_, 0.85f, 1.2f, sr);
            break;
        case FilterType::Presence:
            leftA_.setPeaking(toneHz_, 0.9f, 1.8f, sr);
            rightA_.setPeaking(toneHz_, 0.9f, 1.8f, sr);
            leftB_.setLowPass(3600.0f, 0.707f, sr);
            rightB_.setLowPass(3600.0f, 0.707f, sr);
            break;
        case FilterType::SoftBand:
            // Soft body around carrier — low Q, gentle gain (healing warmth).
            leftA_.setPeaking(toneHz_, 0.8f, 1.6f, sr);
            rightA_.setPeaking(toneHz_, 0.8f, 1.6f, sr);
            leftB_.setLowPass(2600.0f, 0.707f, sr);
            rightB_.setLowPass(2600.0f, 0.707f, sr);
            break;
        case FilterType::Crystal:
            leftA_.setHighPass(90.0f, 0.707f, sr);
            rightA_.setHighPass(90.0f, 0.707f, sr);
            leftB_.setLowPass(4200.0f, 0.707f, sr);
            rightB_.setLowPass(4200.0f, 0.707f, sr);
            break;
        case FilterType::DeepWell:
            leftA_.setLowPass(1400.0f, 0.8f, sr);
            rightA_.setLowPass(1400.0f, 0.8f, sr);
            leftB_.setPeaking(toneHz_, 0.75f, 1.4f, sr);
            rightB_.setPeaking(toneHz_, 0.75f, 1.4f, sr);
            break;
        case FilterType::NotchHum:
            leftA_.setNotch(60.0f, 6.0f, sr);
            rightA_.setNotch(60.0f, 6.0f, sr);
            leftB_.setLowPass(3000.0f, 0.707f, sr);
            rightB_.setLowPass(3000.0f, 0.707f, sr);
            break;
        default:
            // Identity (unity gain)
            leftA_ = Biquad{};
            leftB_ = Biquad{};
            rightA_ = Biquad{};
            rightB_ = Biquad{};
            break;
    }
}

void ColorFilterBank::reset()
{
    leftA_.reset();
    leftB_.reset();
    rightA_.reset();
    rightB_.reset();
    bloomL_.reset();
    bloomR_.reset();
}

void ColorFilterBank::processBloom(AudioFrame& stereo, float mix)
{
    if (!stereo.isStereo() || mix <= 0.0f) {
        return;
    }
    // Full velvet smooth — removes grit while keeping the wave coherent.
    const float wet = clamp(mix, 0.0f, 1.0f);
    const float dry = 1.0f - wet;
    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        const float l = stereo.get(i, 0);
        const float r = stereo.get(i, 1);
        stereo.set(i, 0, dry * l + wet * bloomL_.process(l));
        stereo.set(i, 1, dry * r + wet * bloomR_.process(r));
    }
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
