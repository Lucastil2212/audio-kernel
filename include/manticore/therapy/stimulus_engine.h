#pragma once

#include "manticore/core/audio_frame.h"
#include "manticore/engine/runtime_config.h"
#include "manticore/therapy/therapy_param_set.h"

namespace manticore {

class StimulusEngine {
public:
    StimulusEngine();
    ~StimulusEngine();
    void prepare(const RuntimeConfig& config);
    void setParams(const TherapyParamSet& params);
    void render(AudioFrame& therapyFrame);
    void reset();
    bool isActive() const { return active_; }
    float currentBlend() const { return currentBlend_; }

private:
    RuntimeConfig config_;
    TherapyParamSet params_;
    bool active_ = false;
    float currentBlend_ = 0.0f;
    float rampProgress_ = 0.0f;
    uint64_t samplesRendered_ = 0;

    class Impl;
    Impl* impl_ = nullptr;

    void ensureImpl();
    void destroyImpl();
    float computeRampGain() const;
};

}  // namespace manticore
