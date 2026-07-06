#pragma once

#include "manticore/core/audio_frame.h"

namespace manticore {

void mixFrames(const AudioFrame& a, const AudioFrame& b, AudioFrame& out,
               float gainA, float gainB);

}  // namespace manticore
