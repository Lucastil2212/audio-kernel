#pragma once

#include <array>
#include <vector>

#include "manticore/core/audio_frame.h"
#include "manticore/toneflow/types.h"

namespace manticore::toneflow {

class StereoReverb {
public:
    void prepare(uint32_t sampleRateHz);
    void setPreset(ReverbPreset preset);
    void setWet(float wet);
    void process(AudioFrame& stereo);
    void reset();

    ReverbPreset preset() const { return preset_; }
    float wet() const { return wet_; }
    bool enabled() const { return enabled_; }

private:
    struct Comb {
        std::vector<float> buffer;
        size_t idx = 0;
        float gain = 0.0f;
        float damp = 0.5f;
        float filterStore = 0.0f;

        float process(float x);
        void reset();
    };

    struct Allpass {
        std::vector<float> buffer;
        size_t idx = 0;
        float gain = 0.5f;

        float process(float x);
        void reset();
    };

    uint32_t sampleRateHz_ = 48000;
    ReverbPreset preset_ = ReverbPreset::Off;
    bool enabled_ = false;
    float wet_ = 0.0f;
    float damp_ = 0.6f;
    std::array<Comb, 4> combsL_{};
    std::array<Comb, 4> combsR_{};
    std::array<Allpass, 4> apL_{};
    std::array<Allpass, 4> apR_{};

    void rebuild(float rt60);
};

}  // namespace manticore::toneflow
