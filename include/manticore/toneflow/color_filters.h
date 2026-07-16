#pragma once

#include "manticore/core/audio_frame.h"
#include "manticore/toneflow/types.h"

namespace manticore::toneflow {

/** Stereo color / sculpting filters for therapeutic tone shaping. */
class ColorFilterBank {
public:
    void prepare(uint32_t sampleRateHz);
    void setType(FilterType type);
    void setToneHz(float centerHz);  // follows carrier for adaptive bloom
    void process(AudioFrame& stereo);
    /** Always-on gentle carrier peak — makes pure sines bloom / resonate. */
    void processBloom(AudioFrame& stereo, float mix);
    void reset();

    FilterType type() const { return type_; }

private:
    struct Biquad {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1 = 0, z2 = 0;

        float process(float x);
        void reset();
        void setLowPass(float freq, float q, float sr);
        void setHighPass(float freq, float q, float sr);
        void setBandPass(float freq, float q, float sr);
        void setNotch(float freq, float q, float sr);
        void setPeaking(float freq, float q, float gainDb, float sr);
    };

    uint32_t sampleRateHz_ = 48000;
    FilterType type_ = FilterType::Off;
    float toneHz_ = 210.0f;
    Biquad leftA_{}, leftB_{};
    Biquad rightA_{}, rightB_{};
    Biquad bloomL_{}, bloomR_{};

    void rebuild();
    void rebuildBloom();
};

}  // namespace manticore::toneflow
