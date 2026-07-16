#pragma once

#include "manticore/core/audio_frame.h"
#include "manticore/toneflow/types.h"

namespace manticore::toneflow {

/** Phase-continuous binaural / monaural / isochronic synthesizer. */
class ToneOscillator {
public:
    void reset();

    /** Render stereo tones with per-sample frequency ramping toward targets. */
    void render(AudioFrame& stereoOut, float carrierStart, float carrierEnd,
                float beatStart, float beatEnd, BeatMode mode,
                int harmonicLayers = 1);

    float phaseLeft() const { return static_cast<float>(phaseLeft_); }
    float phaseRight() const { return static_cast<float>(phaseRight_); }

private:
    double phaseLeft_ = 0.0;
    double phaseRight_ = 0.0;
    float harmonicPhases_[3] = {0.0f, 0.0f, 0.0f};
};

}  // namespace manticore::toneflow
