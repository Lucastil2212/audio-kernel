#pragma once

#include <string>

namespace manticore {

enum class CueState {
    Normal,
    SoftBackoff,
    SafeProfile,
    TherapyOff,
    BaselineOnly
};

struct CueHealth {
    CueState state = CueState::BaselineOnly;
    float itdErrorUs = 0.0f;
    float ildErrorDb = 0.0f;
    float coherenceIndex = 1.0f;
    float colorationIndex = 0.0f;
    float headroomDb = 0.0f;
    float syncConfidence = 1.0f;
    std::string stopReason;
};

}  // namespace manticore
