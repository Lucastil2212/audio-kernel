#pragma once

#include "manticore/engine/processor.h"
#include "manticore/safety/safety_governor.h"
#include "manticore/telemetry/system_health.h"
#include "manticore/therapy/stimulus_engine.h"

namespace manticore {

class DspGraph : public AudioProcessor {
public:
    DspGraph();
    ~DspGraph() override;

    void prepare(const RuntimeConfig& config) override;
    void process(AudioFrame& input, AudioFrame& output) override;
    void reset() override;

    void setInputGain(float gain);
    void setOutputGain(float gain);
    void enableLimiter(bool enabled);
    void enableTherapy(bool enabled);
    void setTherapyParams(const TherapyParamSet& params);

    const SystemHealth& health() const { return health_; }
    SafetyGovernor& safetyGovernor() { return safetyGovernor_; }

private:
    RuntimeConfig config_;
    float inputGain_ = 1.0f;
    float outputGain_ = 1.0f;
    bool limiterEnabled_ = true;
    bool therapyEnabled_ = false;
    float therapyBlend_ = 0.0f;

    StimulusEngine stimulusEngine_;
    SafetyGovernor safetyGovernor_;
    SystemHealth health_;

    class Impl;
    Impl* impl_ = nullptr;

    void ensureImpl();
    void destroyImpl();
};

}  // namespace manticore
