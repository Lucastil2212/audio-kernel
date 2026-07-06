#pragma once

#include <cstdint>

namespace manticore {

enum class MotionState {
    Unknown,
    Still,
    Moving,
    Speaking,
    HighMotion
};

struct FitState {
    float fitConfidence = 1.0f;
    float leakIndex = 0.0f;
    float ownVoiceConfidence = 0.0f;
    float referenceMicHealth = 1.0f;
    MotionState motionState = MotionState::Unknown;
    uint64_t calibrationAgeMs = 0;
};

}  // namespace manticore
