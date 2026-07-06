#pragma once

#include <memory>

#include "manticore/engine/dsp_graph.h"
#include "manticore/engine/processor.h"
#include "manticore/engine/runtime_config.h"
#include "manticore/io/audio_backend.h"
#include "manticore/telemetry/system_health.h"

namespace manticore {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool configure(const RuntimeConfig& config);
    bool start();
    void stop();
    SystemHealth getHealth() const;
    DspGraph* graph();

private:
    RuntimeConfig config_;
    std::unique_ptr<AudioBackend> backend_;
    std::unique_ptr<DspGraph> processor_;
    bool configured_ = false;
};

}  // namespace manticore
