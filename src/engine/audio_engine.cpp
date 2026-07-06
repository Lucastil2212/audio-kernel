#include "manticore/engine/audio_engine.h"

#include "manticore/core/errors.h"
#include "manticore/io/portaudio_backend.h"

namespace manticore {

AudioEngine::AudioEngine() : processor_(std::make_unique<DspGraph>()) {}

AudioEngine::~AudioEngine() { stop(); }

bool AudioEngine::configure(const RuntimeConfig& config)
{
    std::string error;
    if (!validateRuntimeConfig(config, error)) {
        return false;
    }

    config_ = config;
    processor_->prepare(config_);
    backend_ = std::make_unique<PortAudioBackend>();
    configured_ = backend_->open(config_, processor_.get());
    return configured_;
}

bool AudioEngine::start()
{
    if (!configured_ || !backend_) {
        return false;
    }
    return backend_->start();
}

void AudioEngine::stop()
{
    if (backend_) {
        backend_->stop();
        backend_->close();
        backend_.reset();
    }
    configured_ = false;
}

SystemHealth AudioEngine::getHealth() const
{
    if (backend_) {
        return backend_->getHealth();
    }
    if (processor_) {
        return processor_->health();
    }
    return SystemHealth{};
}

DspGraph* AudioEngine::graph()
{
    return processor_.get();
}

}  // namespace manticore
