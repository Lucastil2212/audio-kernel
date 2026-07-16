#pragma once

#include "manticore/core/audio_frame.h"
#include "manticore/toneflow/types.h"

namespace manticore::toneflow {

struct PerceptionState {
    float rotationPhase = 0.0f;
    float altPhase = 0.0f;
    float shimmerPhase = 0.0f;
    float breathPhase = 0.0f;
    float expandPhase = 0.0f;
    float latticePhaseA = 0.0f;
    float latticePhaseB = 0.0f;
    float voidPhase = 0.0f;
};

void applyPerception(AudioFrame& stereo, PerceptionMode mode, float beatHz,
                     PerceptionState& state);

}  // namespace manticore::toneflow
