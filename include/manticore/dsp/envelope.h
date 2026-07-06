#pragma once

#include <cmath>

namespace manticore {

class SineEnvelope {
public:
    void set(float rateHz, float depth, float sampleRateHz);
    float next();
    void reset();

private:
    float rateHz_ = 0.05f;
    float depth_ = 0.0f;
    float sampleRateHz_ = 48000.0f;
    float phaseRadians_ = 0.0f;
};

}  // namespace manticore
