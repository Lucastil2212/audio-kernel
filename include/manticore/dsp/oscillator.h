#pragma once

#include <cmath>

#include "manticore/core/audio_frame.h"
#include "manticore/core/constants.h"

namespace manticore {

struct SineOscillator {
    float frequencyHz = 440.0f;
    float amplitude = 0.25f;
    float phaseRadians = 0.0f;

    void render(AudioFrame& frame);
};

}  // namespace manticore
