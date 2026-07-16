#include "manticore/toneflow/stereo_reverb.h"

#include <algorithm>
#include <cmath>

namespace manticore::toneflow {
namespace {

constexpr float kCombMsL[4] = {29.7f, 37.1f, 41.1f, 43.7f};
constexpr float kCombMsR[4] = {31.3f, 38.9f, 42.5f, 44.9f};
constexpr float kApMs[4] = {5.2f, 1.8f, 3.7f, 1.3f};

struct PresetCfg {
    float rt60;
    float wet;
    float damp;
};

PresetCfg cfgFor(ReverbPreset preset)
{
    // Drier / more damped than ToneFlow desktop — protects binaural cues.
    switch (preset) {
        case ReverbPreset::Room:
            return {0.40f, 0.10f, 0.62f};
        case ReverbPreset::Chamber:
            return {0.65f, 0.13f, 0.74f};
        case ReverbPreset::Cave:
            return {1.10f, 0.16f, 0.86f};
        default:
            return {0.0f, 0.0f, 0.0f};
    }
}

}  // namespace

float StereoReverb::Comb::process(float x)
{
    if (buffer.empty()) {
        return x;
    }
    const float delayed = buffer[idx];
    filterStore = delayed * (1.0f - damp) + filterStore * damp;
    const float y = x + gain * filterStore;
    buffer[idx] = y;
    idx = (idx + 1) % buffer.size();
    return y;
}

void StereoReverb::Comb::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    idx = 0;
    filterStore = 0.0f;
}

float StereoReverb::Allpass::process(float x)
{
    if (buffer.empty()) {
        return x;
    }
    const float delayed = buffer[idx];
    const float y = -gain * x + delayed;
    buffer[idx] = x + gain * y;
    idx = (idx + 1) % buffer.size();
    return y;
}

void StereoReverb::Allpass::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    idx = 0;
}

void StereoReverb::prepare(uint32_t sampleRateHz)
{
    sampleRateHz_ = sampleRateHz == 0 ? 48000 : sampleRateHz;
    setPreset(preset_);
}

void StereoReverb::rebuild(float rt60)
{
    const float sr = static_cast<float>(sampleRateHz_);
    for (size_t i = 0; i < combsL_.size(); ++i) {
        const int dL = std::max(1, static_cast<int>(kCombMsL[i] * sr / 1000.0f));
        const int dR = std::max(1, static_cast<int>(kCombMsR[i] * sr / 1000.0f));
        combsL_[i].buffer.assign(static_cast<size_t>(dL), 0.0f);
        combsR_[i].buffer.assign(static_cast<size_t>(dR), 0.0f);
        combsL_[i].idx = 0;
        combsR_[i].idx = 0;
        combsL_[i].filterStore = 0.0f;
        combsR_[i].filterStore = 0.0f;
        combsL_[i].damp = damp_;
        combsR_[i].damp = damp_;
        const float safeRt = std::max(rt60, 0.01f);
        combsL_[i].gain = std::pow(10.0f, -3.0f * static_cast<float>(dL) / sr / safeRt);
        combsR_[i].gain = std::pow(10.0f, -3.0f * static_cast<float>(dR) / sr / safeRt);
    }
    for (size_t i = 0; i < apL_.size(); ++i) {
        const int d = std::max(1, static_cast<int>(kApMs[i] * sr / 1000.0f));
        apL_[i].buffer.assign(static_cast<size_t>(d), 0.0f);
        apR_[i].buffer.assign(static_cast<size_t>(d), 0.0f);
        apL_[i].idx = 0;
        apR_[i].idx = 0;
        apL_[i].gain = 0.5f;
        apR_[i].gain = 0.5f;
    }
}

void StereoReverb::setPreset(ReverbPreset preset)
{
    preset_ = preset;
    const PresetCfg cfg = cfgFor(preset);
    enabled_ = preset != ReverbPreset::Off && cfg.wet > 0.0f;
    wet_ = cfg.wet;
    damp_ = cfg.damp;
    if (enabled_) {
        rebuild(cfg.rt60);
    }
}

void StereoReverb::setWet(float wet)
{
    wet_ = clamp(wet, 0.0f, 0.45f);
}

void StereoReverb::reset()
{
    for (auto& c : combsL_) {
        c.reset();
    }
    for (auto& c : combsR_) {
        c.reset();
    }
    for (auto& a : apL_) {
        a.reset();
    }
    for (auto& a : apR_) {
        a.reset();
    }
}

void StereoReverb::process(AudioFrame& stereo)
{
    if (!enabled_ || wet_ <= 0.0f || !stereo.isStereo()) {
        return;
    }

    const float dry = 1.0f - wet_;
    const size_t n = stereo.framesPerChannel();
    const float invCombs = 1.0f / static_cast<float>(combsL_.size());

    for (size_t i = 0; i < n; ++i) {
        const float xl = stereo.get(i, 0);
        const float xr = stereo.get(i, 1);

        float combL = 0.0f;
        float combR = 0.0f;
        for (size_t c = 0; c < combsL_.size(); ++c) {
            combL += combsL_[c].process(xl);
            combR += combsR_[c].process(xr);
        }
        combL *= invCombs;
        combR *= invCombs;

        for (size_t a = 0; a < apL_.size(); ++a) {
            combL = apL_[a].process(combL);
            combR = apR_[a].process(combR);
        }

        // Minimal cross-mix — keeps L/R more independent for binaural beats.
        const float wl = combL;
        const float wr = combR;
        combL = 0.95f * wl + 0.05f * wr;
        combR = 0.95f * wr + 0.05f * wl;

        stereo.set(i, 0, dry * xl + wet_ * combL);
        stereo.set(i, 1, dry * xr + wet_ * combR);
    }
}

}  // namespace manticore::toneflow
