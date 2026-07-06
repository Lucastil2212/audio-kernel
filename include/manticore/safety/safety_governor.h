#pragma once

#include <vector>

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/analyzer.h"
#include "manticore/engine/runtime_config.h"
#include "manticore/safety/cue_health.h"
#include "manticore/safety/safety_event.h"
#include "manticore/telemetry/fit_state.h"
#include "manticore/telemetry/system_health.h"

namespace manticore {

class SafetyGovernor {
public:
    void prepare(const RuntimeConfig& config);
    CueHealth evaluateFrame(const AudioFrame& frame, const SystemHealth& health);
    void applyFallback(AudioFrame& frame, CueHealth& cueHealth, float& outputGain,
                       float& therapyBlend);
    const std::vector<SafetyEvent>& events() const { return events_; }
    void clearEvents() { events_.clear(); }

private:
    RuntimeConfig config_;
    std::vector<SafetyEvent> events_;
    uint32_t consecutiveClipCount_ = 0;
    uint32_t consecutiveOverrunCount_ = 0;

    void emitEvent(SafetySeverity severity, const std::string& code,
                   const std::string& message, CueState fallback);
    float computeHeadroomDb(float peakAbs) const;
};

class CueGuard {
public:
    void prepare(const RuntimeConfig& config);
    CueHealth evaluate(const AudioFrame& baseFrame, const AudioFrame& candidateFrame,
                       const FitState& fitState);
    void applyFallback(AudioFrame& candidateFrame, const CueHealth& health);

private:
    RuntimeConfig config_;
    float computeHeadroomDb(float peakAbs) const;
};

}  // namespace manticore
