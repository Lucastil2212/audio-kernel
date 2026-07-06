#pragma once

#include "manticore/core/audio_frame.h"
#include "manticore/engine/runtime_config.h"

namespace manticore {

class AudioProcessor {
public:
    virtual ~AudioProcessor() = default;
    virtual void prepare(const RuntimeConfig& config) = 0;
    virtual void process(AudioFrame& input, AudioFrame& output) = 0;
    virtual void reset() = 0;
};

}  // namespace manticore
