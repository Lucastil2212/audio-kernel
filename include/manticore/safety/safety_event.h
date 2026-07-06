#pragma once

#include <cstdint>
#include <string>

#include "manticore/safety/cue_health.h"

namespace manticore {

enum class SafetySeverity {
    Info,
    Warning,
    Error,
    Critical
};

struct SafetyEvent {
    uint64_t timestampNs = 0;
    SafetySeverity severity = SafetySeverity::Info;
    std::string subsystem;
    std::string eventCode;
    std::string message;
    CueState fallbackApplied = CueState::Normal;
};

namespace SafetyEventCode {
constexpr const char* kAudioClipDetected = "AUDIO_CLIP_DETECTED";
constexpr const char* kAudioNanDetected = "AUDIO_NAN_DETECTED";
constexpr const char* kAudioInfDetected = "AUDIO_INF_DETECTED";
constexpr const char* kCallbackOverrun = "CALLBACK_OVERRUN";
constexpr const char* kCallbackUnderrun = "CALLBACK_UNDERRUN";
constexpr const char* kParamStale = "PARAM_STALE";
constexpr const char* kParamOutOfRange = "PARAM_OUT_OF_RANGE";
constexpr const char* kTherapyDisabledByGuard = "THERAPY_DISABLED_BY_GUARD";
constexpr const char* kLimiterEngaged = "LIMITER_ENGAGED";
constexpr const char* kConfigInvalid = "CONFIG_INVALID";
}  // namespace SafetyEventCode

}  // namespace manticore
