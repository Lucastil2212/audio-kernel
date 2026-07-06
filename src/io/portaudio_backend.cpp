#include "manticore/io/portaudio_backend.h"

#include "manticore/core/time.h"
#include "manticore/dsp/analyzer.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <vector>

#ifdef MANTICORE_ENABLE_PORTAUDIO
#include <portaudio.h>
#endif

namespace manticore {

#ifdef MANTICORE_ENABLE_PORTAUDIO

struct PortAudioBackendImpl {
    RuntimeConfig config;
    AudioProcessor* processor = nullptr;
    PaStream* stream = nullptr;
    AtomicSystemHealth atomicHealth;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    AudioFrame inputFrame;
    AudioFrame outputFrame;
    bool opened = false;
    bool running = false;
};

class PortAudioBackend::Impl : public PortAudioBackendImpl {};

static int portAudioCallback(const void* input, void* output,
                             unsigned long frameCount,
                             const PaStreamCallbackTimeInfo* /*timeInfo*/,
                             PaStreamCallbackFlags statusFlags, void* userData)
{
    auto* impl = static_cast<PortAudioBackendImpl*>(userData);
    const auto start = std::chrono::steady_clock::now();

    if (statusFlags & paInputUnderflow) {
        impl->atomicHealth.underflowCount.fetch_add(
            1, std::memory_order_relaxed);
    }
    if (statusFlags & paOutputUnderflow) {
        impl->atomicHealth.underflowCount.fetch_add(
            1, std::memory_order_relaxed);
    }
    if (statusFlags & paInputOverflow || statusFlags & paOutputOverflow) {
        impl->atomicHealth.overrunCount.fetch_add(1, std::memory_order_relaxed);
    }

    const auto* in = static_cast<const float*>(input);
    auto* out = static_cast<float*>(output);
    const size_t sampleCount =
        static_cast<size_t>(frameCount) * impl->config.channels;

    if (in != nullptr) {
        std::memcpy(impl->inputFrame.samples.data(), in,
                    sampleCount * sizeof(float));
    } else {
        std::memset(impl->inputFrame.samples.data(), 0,
                    sampleCount * sizeof(float));
    }

    impl->processor->process(impl->inputFrame, impl->outputFrame);
    std::memcpy(out, impl->outputFrame.samples.data(),
                sampleCount * sizeof(float));

    const FrameStats stats = analyzeFrame(impl->outputFrame);
    impl->atomicHealth.lastPeakAbs.store(stats.peakAbs, std::memory_order_relaxed);
    impl->atomicHealth.lastRms.store(stats.rms, std::memory_order_relaxed);
    impl->atomicHealth.callbackCount.fetch_add(1, std::memory_order_relaxed);

    if (impl->outputFrame.clipFlag) {
        impl->atomicHealth.clipCount.fetch_add(1, std::memory_order_relaxed);
    }

    const auto end = std::chrono::steady_clock::now();
    const float durationMs =
        static_cast<float>(
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count()) /
        1000.0f;
    impl->atomicHealth.callbackDurationMs.store(durationMs,
                                                std::memory_order_relaxed);

    return paContinue;
}

PortAudioBackend::PortAudioBackend() : impl_(new Impl()) {}

PortAudioBackend::~PortAudioBackend()
{
    close();
    delete impl_;
    impl_ = nullptr;
}

bool PortAudioBackend::open(const RuntimeConfig& config,
                            AudioProcessor* processor)
{
    if (!impl_ || !processor) {
        return false;
    }

    impl_->config = config;
    impl_->processor = processor;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        return false;
    }

    const size_t sampleCount =
        static_cast<size_t>(config.frameSize) * config.channels;
    impl_->inputBuffer.assign(sampleCount, 0.0f);
    impl_->outputBuffer.assign(sampleCount, 0.0f);
    impl_->inputFrame =
        AudioFrame(config.frameSize, config.sampleRateHz, config.channels);
    impl_->outputFrame =
        AudioFrame(config.frameSize, config.sampleRateHz, config.channels);

    PaStreamParameters inputParams{};
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        Pa_Terminate();
        return false;
    }
    inputParams.channelCount = static_cast<int>(config.channels);
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency =
        Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    PaStreamParameters outputParams{};
    outputParams.device = Pa_GetDefaultOutputDevice();
    if (outputParams.device == paNoDevice) {
        Pa_Terminate();
        return false;
    }
    outputParams.channelCount = static_cast<int>(config.channels);
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency =
        Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&impl_->stream, &inputParams, &outputParams,
                        static_cast<double>(config.sampleRateHz),
                        config.frameSize, paClipOff, portAudioCallback, impl_);
    if (err != paNoError) {
        Pa_Terminate();
        return false;
    }

    impl_->opened = true;
    return true;
}

bool PortAudioBackend::start()
{
    if (!impl_ || !impl_->opened) {
        return false;
    }
    const PaError err = Pa_StartStream(impl_->stream);
    if (err != paNoError) {
        return false;
    }
    impl_->running = true;
    return true;
}

void PortAudioBackend::stop()
{
    if (!impl_ || !impl_->running) {
        return;
    }
    Pa_StopStream(impl_->stream);
    impl_->running = false;
}

void PortAudioBackend::close()
{
    if (!impl_) {
        return;
    }
    stop();
    if (impl_->stream) {
        Pa_CloseStream(impl_->stream);
        impl_->stream = nullptr;
    }
    if (impl_->opened) {
        Pa_Terminate();
        impl_->opened = false;
    }
}

SystemHealth PortAudioBackend::getHealth() const
{
    if (!impl_) {
        return SystemHealth{};
    }
    return impl_->atomicHealth.snapshot();
}

#else

class PortAudioBackend::Impl {};

PortAudioBackend::PortAudioBackend() : impl_(new Impl()) {}

PortAudioBackend::~PortAudioBackend()
{
    delete impl_;
    impl_ = nullptr;
}

bool PortAudioBackend::open(const RuntimeConfig& /*config*/,
                            AudioProcessor* /*processor*/)
{
    return false;
}

bool PortAudioBackend::start() { return false; }

void PortAudioBackend::stop() {}

void PortAudioBackend::close() {}

SystemHealth PortAudioBackend::getHealth() const { return SystemHealth{}; }

#endif

}  // namespace manticore
