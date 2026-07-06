#pragma once

#include "manticore/core/audio_frame.h"

namespace manticore {

float clampSample(float sample, float minValue = -1.0f, float maxValue = 1.0f);
void applyLimiter(AudioFrame& frame, float threshold = 1.0f);

}  // namespace manticore
