#include "manticore/safety/safety_governor.h"

#include "manticore/core/time.h"
#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/gain.h"
#include "manticore/dsp/limiter.h"

#include <cmath>

namespace manticore {

void SafetyGovernor::prepare(const RuntimeConfig& config)
{
    config_ = config;
    events_.clear();
    consecutiveClipCount_ = 0;
    consecutiveOverrunCount_ = 0;
}

float SafetyGovernor::computeHeadroomDb(float peakAbs) const
{
    if (peakAbs <= 0.0f) {
        return 120.0f;
    }
    return 20.0f * std::log10(1.0f / peakAbs);
}

void SafetyGovernor::emitEvent(SafetySeverity severity, const std::string& code,
                               const std::string& message, CueState fallback)
{
    SafetyEvent event;
    event.timestampNs = monotonicTimeNs();
    event.severity = severity;
    event.subsystem = "safety_governor";
    event.eventCode = code;
    event.message = message;
    event.fallbackApplied = fallback;
    events_.push_back(event);
}

CueHealth SafetyGovernor::evaluateFrame(const AudioFrame& frame,
                                        const SystemHealth& health)
{
    CueHealth cueHealth;
    const FrameStats stats = analyzeFrame(frame);

    cueHealth.headroomDb = computeHeadroomDb(stats.peakAbs);

    if (stats.hasNaN) {
        cueHealth.state = CueState::BaselineOnly;
        cueHealth.stopReason = "NaN detected";
        emitEvent(SafetySeverity::Critical, SafetyEventCode::kAudioNanDetected,
                  "NaN samples detected", CueState::BaselineOnly);
        return cueHealth;
    }

    if (stats.hasInf) {
        cueHealth.state = CueState::BaselineOnly;
        cueHealth.stopReason = "Inf detected";
        emitEvent(SafetySeverity::Critical, SafetyEventCode::kAudioInfDetected,
                  "Inf samples detected", CueState::BaselineOnly);
        return cueHealth;
    }

    if (health.overrunCount > 0) {
        ++consecutiveOverrunCount_;
        if (consecutiveOverrunCount_ >= 3) {
            cueHealth.state = CueState::SafeProfile;
            cueHealth.stopReason = "callback overruns";
            emitEvent(SafetySeverity::Warning, SafetyEventCode::kCallbackOverrun,
                      "Repeated callback overruns", CueState::SafeProfile);
            return cueHealth;
        }
    } else {
        consecutiveOverrunCount_ = 0;
    }

    if (health.underflowCount > 0) {
        emitEvent(SafetySeverity::Warning, SafetyEventCode::kCallbackUnderrun,
                  "Callback underrun detected", CueState::Normal);
    }

    if (stats.peakAbs > 1.2f) {
        cueHealth.state = CueState::SafeProfile;
        cueHealth.stopReason = "peak exceeds safe threshold";
        return cueHealth;
    }

    if (stats.clipped || frame.clipFlag) {
        ++consecutiveClipCount_;
        emitEvent(SafetySeverity::Warning, SafetyEventCode::kAudioClipDetected,
                  "Clipping detected", CueState::SoftBackoff);
        if (consecutiveClipCount_ >= 3) {
            cueHealth.state = CueState::TherapyOff;
            cueHealth.stopReason = "repeated clipping";
            emitEvent(SafetySeverity::Error,
                      SafetyEventCode::kTherapyDisabledByGuard,
                      "Therapy disabled due to repeated clipping",
                      CueState::TherapyOff);
            return cueHealth;
        }
        cueHealth.state = CueState::SoftBackoff;
        cueHealth.stopReason = "clipping";
        return cueHealth;
    }

    consecutiveClipCount_ = 0;
    cueHealth.state = CueState::Normal;
    return cueHealth;
}

void SafetyGovernor::applyFallback(AudioFrame& frame, CueHealth& cueHealth,
                                   float& outputGain, float& therapyBlend)
{
    switch (cueHealth.state) {
        case CueState::Normal:
            break;
        case CueState::SoftBackoff:
            outputGain *= 0.75f;
            therapyBlend *= 0.5f;
            emitEvent(SafetySeverity::Info, SafetyEventCode::kLimiterEngaged,
                      "Soft backoff applied", CueState::SoftBackoff);
            break;
        case CueState::SafeProfile:
            therapyBlend = 0.0f;
            outputGain = std::min(outputGain, 1.0f);
            break;
        case CueState::TherapyOff:
            therapyBlend = 0.0f;
            emitEvent(SafetySeverity::Warning,
                      SafetyEventCode::kTherapyDisabledByGuard,
                      "Therapy blend zeroed", CueState::TherapyOff);
            break;
        case CueState::BaselineOnly:
            therapyBlend = 0.0f;
            outputGain = 1.0f;
            frame.samples = frame.samples;
            applyLimiter(frame);
            break;
    }
}

void CueGuard::prepare(const RuntimeConfig& config) { config_ = config; }

float CueGuard::computeHeadroomDb(float peakAbs) const
{
    if (peakAbs <= 0.0f) {
        return 120.0f;
    }
    return 20.0f * std::log10(1.0f / peakAbs);
}

CueHealth CueGuard::evaluate(const AudioFrame& /*baseFrame*/,
                             const AudioFrame& candidateFrame,
                             const FitState& fitState)
{
    CueHealth health;
    const FrameStats stats = analyzeFrame(candidateFrame);
    health.headroomDb = computeHeadroomDb(stats.peakAbs);
    health.syncConfidence = fitState.fitConfidence;

    if (stats.hasNaN || stats.hasInf) {
        health.state = CueState::BaselineOnly;
        health.stopReason = "invalid samples";
        return health;
    }
    if (stats.peakAbs > 1.2f) {
        health.state = CueState::SafeProfile;
        return health;
    }
    if (stats.peakAbs > 1.0f || stats.clipped) {
        health.state = CueState::SoftBackoff;
        return health;
    }
    health.state = CueState::Normal;
    return health;
}

void CueGuard::applyFallback(AudioFrame& candidateFrame, const CueHealth& health)
{
    if (health.state == CueState::BaselineOnly ||
        health.state == CueState::SafeProfile) {
        applyLimiter(candidateFrame);
    }
}

}  // namespace manticore
