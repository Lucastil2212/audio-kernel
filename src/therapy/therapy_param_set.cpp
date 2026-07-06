#include "manticore/therapy/therapy_param_set.h"

#include "manticore/core/time.h"

#include <sstream>

namespace manticore {

bool validateTherapyParams(const TherapyParamSet& params, uint32_t sampleRateHz,
                           uint64_t nowNs, std::string& errorMessage)
{
    if (params.bandLowHz < 20.0f) {
        errorMessage = "bandLowHz must be >= 20 Hz";
        return false;
    }
    if (params.bandHighHz > static_cast<float>(sampleRateHz) / 2.0f) {
        errorMessage = "bandHighHz must be <= Nyquist";
        return false;
    }
    if (params.bandLowHz >= params.bandHighHz) {
        errorMessage = "bandLowHz must be < bandHighHz";
        return false;
    }
    if (params.blendGain < 0.0f || params.blendGain > 1.0f) {
        errorMessage = "blendGain must be in [0, 1]";
        return false;
    }
    if (params.envelopeDepth < 0.0f || params.envelopeDepth > 1.0f) {
        errorMessage = "envelopeDepth must be in [0, 1]";
        return false;
    }
    if (params.validUntilTimestampNs != 0 &&
        nowNs > params.validUntilTimestampNs) {
        errorMessage = "therapy parameters are stale";
        return false;
    }
    return true;
}

}  // namespace manticore
