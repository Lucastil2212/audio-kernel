#pragma once

#include "manticore/core/audio_frame.h"

namespace manticore {

class OnePoleLowPass {
public:
    void setCutoff(float cutoffHz, float sampleRateHz);
    void reset();
    float processSample(float x);
    void processFrame(AudioFrame& frame);

private:
    float alpha_ = 0.0f;
    float z1_ = 0.0f;
};

class OnePoleHighPass {
public:
    void setCutoff(float cutoffHz, float sampleRateHz);
    void reset();
    float processSample(float x);
    void processFrame(AudioFrame& frame);

private:
    float alpha_ = 0.0f;
    float previousInput_ = 0.0f;
    float previousOutput_ = 0.0f;
};

}  // namespace manticore
