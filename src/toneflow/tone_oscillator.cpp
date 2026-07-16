#include "manticore/toneflow/tone_oscillator.h"

#include <cmath>

namespace manticore::toneflow {
namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

void wrapPhaseInPlace(double& phase)
{
    while (phase >= kTwoPi) {
        phase -= kTwoPi;
    }
    while (phase < 0.0) {
        phase += kTwoPi;
    }
}

}  // namespace

void ToneOscillator::reset()
{
    phaseLeft_ = 0.0;
    phaseRight_ = 0.0;
    for (float& p : harmonicPhases_) {
        p = 0.0f;
    }
}

void ToneOscillator::render(AudioFrame& stereoOut, float carrierStart,
                            float carrierEnd, float beatStart, float beatEnd,
                            BeatMode mode, int /*harmonicLayers*/)
{
    if (!stereoOut.isStereo() || stereoOut.sampleRateHz == 0) {
        return;
    }

    // Lower peak leaves headroom for a soft sub hum without clipping/static.
    constexpr float kBinauralAmp = 0.58f;
    constexpr float kMonauralAmp = 0.32f;
    constexpr float kIsochronicAmp = 0.52f;

    const size_t n = stereoOut.framesPerChannel();
    const double sr = static_cast<double>(stereoOut.sampleRateHz);
    const float invN = n > 1 ? 1.0f / static_cast<float>(n - 1) : 0.0f;

    carrierStart = clamp(carrierStart, 100.0f, 1000.0f);
    carrierEnd = clamp(carrierEnd, 100.0f, 1000.0f);
    beatStart = clamp(beatStart, 0.0f, 40.0f);
    beatEnd = clamp(beatEnd, 0.0f, 40.0f);

    if (mode == BeatMode::Isochronic) {
        beatStart = clamp(std::max(beatStart, 0.1f), 0.1f, 40.0f);
        beatEnd = clamp(std::max(beatEnd, 0.1f), 0.1f, 40.0f);
    }

    const float loudness =
        0.5f * (equalLoudnessGain(carrierStart) + equalLoudnessGain(carrierEnd));

    for (size_t i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) * invN;
        const float carrier = lerp(carrierStart, carrierEnd, t);
        const float beat = lerp(beatStart, beatEnd, t);

        float left = 0.0f;
        float right = 0.0f;

        if (mode == BeatMode::Isochronic) {
            const double cInc = kTwoPi * static_cast<double>(carrier) / sr;
            const double bInc = kTwoPi * static_cast<double>(beat) / sr;
            // Soft gate with high floor — never fully chops (avoids clicky static).
            const float gate =
                0.35f + 0.65f * (0.5f - 0.5f * static_cast<float>(std::cos(phaseRight_)));
            const float mono = static_cast<float>(std::sin(phaseLeft_)) * gate *
                               kIsochronicAmp * loudness;
            left = mono;
            right = mono;
            phaseLeft_ += cInc;
            phaseRight_ += bInc;
        } else if (mode == BeatMode::Monaural) {
            const double fL =
                static_cast<double>(carrier) - static_cast<double>(beat) * 0.5;
            const double fR =
                static_cast<double>(carrier) + static_cast<double>(beat) * 0.5;
            const float mono =
                (static_cast<float>(std::sin(phaseLeft_)) +
                 static_cast<float>(std::sin(phaseRight_))) *
                kMonauralAmp * loudness;
            left = mono;
            right = mono;
            phaseLeft_ += kTwoPi * fL / sr;
            phaseRight_ += kTwoPi * fR / sr;
        } else {
            // Classic binaural: one pure sine per ear.
            const double fL =
                static_cast<double>(carrier) - static_cast<double>(beat) * 0.5;
            const double fR =
                static_cast<double>(carrier) + static_cast<double>(beat) * 0.5;
            left =
                static_cast<float>(std::sin(phaseLeft_)) * kBinauralAmp * loudness;
            right =
                static_cast<float>(std::sin(phaseRight_)) * kBinauralAmp * loudness;
            phaseLeft_ += kTwoPi * fL / sr;
            phaseRight_ += kTwoPi * fR / sr;
        }

        wrapPhaseInPlace(phaseLeft_);
        wrapPhaseInPlace(phaseRight_);

        stereoOut.set(i, 0, left);
        stereoOut.set(i, 1, right);
    }
}

}  // namespace manticore::toneflow
