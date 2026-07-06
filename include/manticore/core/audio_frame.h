#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include "manticore/core/audio_types.h"

namespace manticore {

struct AudioFrame {
    std::vector<float> samples;
    uint32_t sampleRateHz = 48000;
    uint32_t channels = 1;
    uint64_t frameIndex = 0;
    uint64_t timestampNs = 0;
    bool clipFlag = false;
    bool overflowFlag = false;
    bool underflowFlag = false;

    AudioFrame() = default;

    AudioFrame(uint32_t framesPerChannel, uint32_t rateHz, uint32_t channelCount)
        : samples(static_cast<size_t>(framesPerChannel) * channelCount, 0.0f),
          sampleRateHz(rateHz),
          channels(channelCount)
    {
    }

    size_t sampleCount() const { return samples.size(); }

    size_t framesPerChannel() const
    {
        if (channels == 0) {
            return 0;
        }
        return samples.size() / channels;
    }

    float durationMs() const
    {
        if (sampleRateHz == 0) {
            return 0.0f;
        }
        return static_cast<float>(framesPerChannel()) /
               static_cast<float>(sampleRateHz) * 1000.0f;
    }

    bool isMono() const { return channels == 1; }

    bool isStereo() const { return channels == 2; }

    float get(size_t frame, size_t channel) const
    {
        return samples[frame * channels + channel];
    }

    void set(size_t frame, size_t channel, float value)
    {
        samples[frame * channels + channel] = value;
    }

    void clear()
    {
        for (float& sample : samples) {
            sample = 0.0f;
        }
        clipFlag = false;
        overflowFlag = false;
        underflowFlag = false;
    }

    bool hasInvalidSamples() const
    {
        for (float sample : samples) {
            if (std::isnan(sample) || std::isinf(sample)) {
                return true;
            }
        }
        return false;
    }
};

}  // namespace manticore
