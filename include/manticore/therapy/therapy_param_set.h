#pragma once

#include <cstdint>
#include <string>

#include "manticore/therapy/carrier_family.h"

namespace manticore {

enum class SessionStage {
    Disabled,
    RampIn,
    Active,
    RampOut,
    Complete
};

struct TherapyParamSet {
    uint32_t schemaVersion = 1;
    uint32_t profileId = 0;
    CarrierFamily carrierFamily = CarrierFamily::None;
    float bandLowHz = 200.0f;
    float bandHighHz = 8000.0f;
    float blendGain = 0.0f;
    float envelopeRateHz = 0.05f;
    float envelopeDepth = 0.0f;
    float itdUs = 0.0f;
    float ildDb = 0.0f;
    uint32_t phaseRule = 0;
    SessionStage stage = SessionStage::Disabled;
    uint64_t validUntilTimestampNs = 0;
    uint32_t seed = 1;
};

bool validateTherapyParams(const TherapyParamSet& params, uint32_t sampleRateHz,
                           uint64_t nowNs, std::string& errorMessage);

}  // namespace manticore
