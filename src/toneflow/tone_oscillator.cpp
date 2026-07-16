#include "manticore/toneflow/tone_oscillator.h"

#include <cmath>

namespace manticore::toneflow {
namespace {

constexpr float kTwoPi = 6.28318530717958647692f;
// Slightly lower peak than ToneFlow desktop — long-session comfort.
constexpr float kBinauralAmp = 0.72f;
constexpr float kMonauralAmp = 0.38f;
constexpr float kIsochronicAmp = 0.62f;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

/** Raised-cosine^2 gate — softer edges, less startle than plain cosine. */
float softIsochronicGate(float phase)
{
    const float raised = 0.5f - 0.5f * std::cos(phase);
    return raised * raised;
}

}  // namespace

void ToneOscillator::reset()
{
    phaseLeft_ = 0.0f;
    phaseRight_ = 0.0f;
    for (float& p : harmonicPhases_) {
        p = 0.0f;
    }
}

float ToneOscillator::wrapPhase(float phase)
{
    while (phase >= kTwoPi) {
        phase -= kTwoPi;
    }
    while (phase < 0.0f) {
        phase += kTwoPi;
    }
    return phase;
}

void ToneOscillator::render(AudioFrame& stereoOut, float carrierStart,
                            float carrierEnd, float beatStart, float beatEnd,
                            BeatMode mode, int harmonicLayers)
{
    if (!stereoOut.isStereo() || stereoOut.sampleRateHz == 0) {
        return;
    }

    const size_t n = stereoOut.framesPerChannel();
    const float sr = static_cast<float>(stereoOut.sampleRateHz);
    const float invN = n > 1 ? 1.0f / static_cast<float>(n - 1) : 0.0f;

    carrierStart = clamp(carrierStart, 100.0f, 1000.0f);
    carrierEnd = clamp(carrierEnd, 100.0f, 1000.0f);
    beatStart = clamp(beatStart, 0.0f, 40.0f);
    beatEnd = clamp(beatEnd, 0.0f, 40.0f);
    harmonicLayers = harmonicLayers < 1 ? 1 : (harmonicLayers > 3 ? 3 : harmonicLayers);

    if (mode == BeatMode::Isochronic) {
        beatStart = clamp(std::max(beatStart, 0.1f), 0.1f, 40.0f);
        beatEnd = clamp(std::max(beatEnd, 0.1f), 0.1f, 40.0f);
    }

    // Warm harmonic bed: octave + perfect fifth, kept quiet so beat stays clear.
    constexpr float kLayerRatio[3] = {1.0f, 1.5f, 2.0f};
    constexpr float kLayerAmp[3] = {0.90f, 0.07f, 0.03f};
    float ampSum = 0.0f;
    for (int i = 0; i < harmonicLayers; ++i) {
        ampSum += kLayerAmp[i];
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
            const float cInc = kTwoPi * carrier / sr;
            const float bInc = kTwoPi * beat / sr;
            const float carrierSample = std::sin(phaseLeft_);
            const float gate = softIsochronicGate(phaseRight_);
            // Floor keeps a faint carrier so pulses feel less abrupt.
            const float mono =
                carrierSample * (0.12f + 0.88f * gate) * kIsochronicAmp * loudness;
            left = mono;
            right = mono;
            phaseLeft_ = wrapPhase(phaseLeft_ + cInc);
            phaseRight_ = wrapPhase(phaseRight_ + bInc);
        } else if (mode == BeatMode::Monaural) {
            const float fL = carrier - beat * 0.5f;
            const float fR = carrier + beat * 0.5f;
            const float mono = (std::sin(phaseLeft_) + std::sin(phaseRight_)) *
                               kMonauralAmp * loudness;
            left = mono;
            right = mono;
            phaseLeft_ = wrapPhase(phaseLeft_ + kTwoPi * fL / sr);
            phaseRight_ = wrapPhase(phaseRight_ + kTwoPi * fR / sr);
        } else {
            // Binaural: independent L/R sines — keep channels pure for IACD.
            const float fL = carrier - beat * 0.5f;
            const float fR = carrier + beat * 0.5f;
            left = std::sin(phaseLeft_) * kLayerAmp[0];
            right = std::sin(phaseRight_) * kLayerAmp[0];
            phaseLeft_ = wrapPhase(phaseLeft_ + kTwoPi * fL / sr);
            phaseRight_ = wrapPhase(phaseRight_ + kTwoPi * fR / sr);

            for (int h = 1; h < harmonicLayers; ++h) {
                const float freq = clamp(carrier * kLayerRatio[h], 100.0f, 4000.0f);
                const float tone = std::sin(harmonicPhases_[h]) * kLayerAmp[h];
                // Shared harmonic bed (no extra beat) — warmth without interference.
                left += tone;
                right += tone;
                harmonicPhases_[h] =
                    wrapPhase(harmonicPhases_[h] + kTwoPi * freq / sr);
            }

            left = (left / ampSum) * kBinauralAmp * loudness;
            right = (right / ampSum) * kBinauralAmp * loudness;
        }

        stereoOut.set(i, 0, left);
        stereoOut.set(i, 1, right);
    }
}

}  // namespace manticore::toneflow
