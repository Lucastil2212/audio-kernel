#pragma once

#include "manticore/core/audio_frame.h"

namespace manticore {

void applyGain(AudioFrame& frame, float gainLinear);

}  // namespace manticore
