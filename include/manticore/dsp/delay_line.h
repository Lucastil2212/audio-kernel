#pragma once

#include <cstddef>
#include <vector>

#include "manticore/core/audio_frame.h"

namespace manticore {

class DelayLine {
public:
    void prepare(size_t maxDelaySamples);
    void setDelaySamples(size_t delaySamples);
    void reset();
    float processSample(float x);
    void processFrame(AudioFrame& frame);

private:
    std::vector<float> buffer_;
    size_t writeIndex_ = 0;
    size_t delaySamples_ = 0;
};

}  // namespace manticore
