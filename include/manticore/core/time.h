#pragma once

#include <cstdint>

namespace manticore {

uint64_t monotonicTimeNs();

float samplesToMs(uint32_t sampleCount, uint32_t sampleRateHz);

uint32_t msToSamples(float durationMs, uint32_t sampleRateHz);

}  // namespace manticore
