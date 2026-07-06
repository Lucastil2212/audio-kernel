#pragma once

#include <cstdint>

#include "manticore/core/audio_frame.h"

namespace manticore {

struct WhiteNoiseGenerator {
    uint32_t seed = 1;
    float amplitude = 0.1f;

    void render(AudioFrame& frame);
    void reset() { state_ = seed; }
    float nextSample();

private:
    uint32_t state_ = 1;
};

struct PinkNoiseGenerator {
    uint32_t seed = 1;
    float amplitude = 0.1f;

    void render(AudioFrame& frame);
    void reset();

private:
    WhiteNoiseGenerator white_;
    float b0_ = 0.0f;
    float b1_ = 0.0f;
    float b2_ = 0.0f;
    float b3_ = 0.0f;
    float b4_ = 0.0f;
    float b5_ = 0.0f;
    float b6_ = 0.0f;
};

}  // namespace manticore
