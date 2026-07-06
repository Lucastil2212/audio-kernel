#pragma once

#include "manticore/io/audio_backend.h"

namespace manticore {

class PortAudioBackend : public AudioBackend {
public:
    PortAudioBackend();
    ~PortAudioBackend() override;

    bool open(const RuntimeConfig& config, AudioProcessor* processor) override;
    bool start() override;
    void stop() override;
    void close() override;
    SystemHealth getHealth() const override;

private:
    class Impl;
    Impl* impl_ = nullptr;
};

}  // namespace manticore
