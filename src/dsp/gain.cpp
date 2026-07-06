#include "manticore/dsp/gain.h"

namespace manticore {

void applyGain(AudioFrame& frame, float gainLinear)
{
    for (float& sample : frame.samples) {
        sample *= gainLinear;
    }
}

}  // namespace manticore
