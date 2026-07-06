#pragma once

#include "manticore/engine/processor.h"
#include "manticore/engine/runtime_config.h"
#include "manticore/telemetry/system_health.h"

namespace manticore {

class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    virtual bool open(const RuntimeConfig& config, AudioProcessor* processor) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void close() = 0;
    virtual SystemHealth getHealth() const = 0;
};

}  // namespace manticore
