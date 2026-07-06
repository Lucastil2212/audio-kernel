#include "manticore/dsp/delay_line.h"

namespace manticore {

void DelayLine::prepare(size_t maxDelaySamples)
{
    buffer_.assign(maxDelaySamples + 1, 0.0f);
    writeIndex_ = 0;
    delaySamples_ = 0;
}

void DelayLine::setDelaySamples(size_t delaySamples)
{
    if (buffer_.empty()) {
        delaySamples_ = 0;
        return;
    }
    delaySamples_ = delaySamples;
    if (delaySamples_ >= buffer_.size()) {
        delaySamples_ = buffer_.size() - 1;
    }
}

void DelayLine::reset()
{
    for (float& sample : buffer_) {
        sample = 0.0f;
    }
    writeIndex_ = 0;
}

float DelayLine::processSample(float x)
{
    if (buffer_.empty()) {
        return x;
    }

    buffer_[writeIndex_] = x;
    const size_t readIndex =
        (writeIndex_ + buffer_.size() - delaySamples_) % buffer_.size();
    const float output = buffer_[readIndex];
    writeIndex_ = (writeIndex_ + 1) % buffer_.size();
    return output;
}

void DelayLine::processFrame(AudioFrame& frame)
{
    for (float& sample : frame.samples) {
        sample = processSample(sample);
    }
}

}  // namespace manticore
