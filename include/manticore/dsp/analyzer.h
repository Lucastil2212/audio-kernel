#pragma once

#include "manticore/core/audio_frame.h"

namespace manticore {

struct FrameStats {
    float minSample = 0.0f;
    float maxSample = 0.0f;
    float peakAbs = 0.0f;
    float rms = 0.0f;
    float dcOffset = 0.0f;
    bool clipped = false;
    bool hasNaN = false;
    bool hasInf = false;
};

FrameStats analyzeFrame(const AudioFrame& frame);

}  // namespace manticore
